use std::path::Path;
use std::env;
use std::error::Error;

use std::os::unix::prelude::RawFd;
use std::process::exit;

use knn_ocr::{read_labeled_data, knn, LabeledFeatures};

use nix::unistd::{fork, ForkResult, pipe, close, Pid, read, write, getppid};
use nix::sys::wait::waitpid;
use nix::sys::signal::{signal, kill, SIGTERM, SigHandler};

const TEST_DATA: &str = "t10k-images-idx3-ubyte";
const TEST_LABELS: &str  = "t10k-labels-idx1-ubyte";
const TRAINING_DATA: &str  = "train-images-idx3-ubyte";
const TRAINING_LABELS: &str  = "train-labels-idx1-ubyte";

fn usage(prog_path: &str, err: &str) -> ! {
    let prog_name =
	Path::new(prog_path).file_name().unwrap().to_str().unwrap();
    eprintln!("{}\nusage: {} DATA_DIR [N_TESTS [K [N_PROC]]]", err, prog_name);
    std::process::exit(1);
}

struct Args {
    data_dir: String,
    k: usize,
    n_test: i32,
    n_proc: usize,
}

impl Args {
    fn new(argv: &Vec<String>) -> Result<Args, Box<dyn Error>> {
    	let argc = argv.len();
    	if argc < 2 || argc > 5 {
    	    Err("must be called with 1 ... 4 args")?;
    	}
    	let data_dir = argv[1].clone();
    	let mut n_test: i32 = -1;
    	let mut k: usize = 3;
        let mut n_proc: usize = 4;
    	if argc > 2 {
    	    n_test = argv[2].parse::<i32>()?;
    	}
    	if argc > 3 {
    	    k = argv[3].parse::<usize>()?;
    	    if k == 0 {
    		    Err("k must be positive")?;
    	    }
    	}
    	if argc > 4 {
    	    n_proc = argv[4].parse::<usize>()?;
    	    if n_proc == 0 {
    		    Err("n_proc must be positive")?;
    	    }
    	}
    	Ok(Args { data_dir, k, n_test, n_proc })
    }
}

static mut CHILD_PIDS: Vec<Pid> = Vec::new(); // necessary global to get this info to signal handler

extern "C" fn kill_children(_x: i32) {
    for i in 0..unsafe{CHILD_PIDS.len()} {
        unsafe{kill(CHILD_PIDS[i], SIGTERM)}.expect(&format!("Failed to kill child with PID {}", unsafe{CHILD_PIDS[i]}));
    }
    exit(0); // kill self
}

fn do_child(train_data: &Vec<LabeledFeatures>, send_result: RawFd, get_data: RawFd, proc_id: usize, args: &Args) {
    let mut size: [u8; 8] = [0; 8];
        //.expect;
    match read(get_data, &mut size) {
        Ok(result) => result,
        Err(err) => { 
            kill(getppid(), SIGTERM).unwrap();
            panic!("Error: {:?}", err);
        }
    };
    let n = usize::from_le_bytes(size[0..8].try_into().expect("Conversion failed in child"));
    let mut img_num = n / args.n_proc;
    if n % args.n_proc > proc_id { 
        img_num += 1;
    }
    //println!("for proc{}: {}", proc_id, img_num);
    let mut send: [u8; 10] = [0; 10];
    let mut img: [u8; 785] = [0; 785];
    for _i in 0..img_num {
        match read(get_data, &mut img) {
            Ok(result) => result,
            Err(err) => { 
                kill(getppid(), SIGTERM).unwrap();
                panic!("Error: {:?}", err);
            }
        };
	    let nearest_index = knn(&train_data, &img[1..].to_vec(), args.k);
	    let predicted = train_data[nearest_index].label;
	    let expected = img[0]; // img[0] is the label
        let ni_array = nearest_index.to_le_bytes();
        for j in 0..ni_array.len() {
            send[j] = ni_array[j];
        }
        send[8] = predicted;
        send[9] = expected;
        match write(send_result, &send) {
            Ok(result) => result,
            Err(err) => { 
                kill(getppid(), SIGTERM).unwrap();
                panic!("Error: {:?}", err);
            }
        };
    }
    close(send_result).expect("Failed to close file descriptor");
    close(get_data).expect("Failed to close file descriptor");
    exit(0);
}

fn main() {
    let argv: Vec<String> = env::args().collect();
    let args;
    match Args::new(&argv) {
    	Err(err) => usage(&argv[0], &err.to_string()),
    	Ok(a) => args = a,
    };
    let train_data = read_labeled_data(&args.data_dir, TRAINING_DATA, TRAINING_LABELS);

    // Spawn worker processes and their pipes, printing out (and recording) PIDs
    let mut c2p_pipes = Vec::new();
    let mut p2c_pipes = Vec::new();
    let sig_handler = SigHandler::Handler(kill_children);
    unsafe{signal(SIGTERM, sig_handler)}.expect("Signal failed");
    for i in 0..args.n_proc {
        c2p_pipes.push(pipe().expect("Failed to create pipe"));
        p2c_pipes.push(pipe().expect("Failed to create pipe"));
        match unsafe{fork()} {
            Ok(ForkResult::Parent{child}) => {
                println!("Process with PID {} created", child);
                unsafe{CHILD_PIDS.push(child)};
                close(c2p_pipes[i].1).expect("Failed to close file descriptor");
                close(p2c_pipes[i].0).expect("Failed to close file descriptor");
            }
            Ok(ForkResult::Child) => {
                close(c2p_pipes[i].0).expect(&format!("failed to close fd for i={}", i));
                close(p2c_pipes[i].1).expect("Failed to close file descriptor");
                do_child(&train_data, c2p_pipes[i].1, p2c_pipes[i].0, i, &args);
            }
            Err(_) => {
                println!("Fork failed, killing children and terminating");
                kill_children(16); // have kill any child process that may have already been created
            }
        }
    }
    
    let test_data = read_labeled_data(&args.data_dir, TEST_DATA, TEST_LABELS);
    let n: usize = if args.n_test <= 0 {
	    test_data.len()
    } else {
	    args.n_test as usize
    };
    
    // send img to each worker, read result over pipe
    let mut ok = 0;
    let mut buf: [u8; 10] = [0; 10];
    let mut img: [u8; 785] = [0; 785]; //index 0 = label, indexes 1..784 = image data
    for i in 0..args.n_proc { // send size to each worker first
        match write(p2c_pipes[i].1, &n.to_le_bytes()) {
            Ok(result) => result,
            Err(err) => {
                kill_children(15);
                panic!("Error: {:?}", err);
            }
        };
    }
    for i in 0..n {
        img[0] = test_data[i].label;
        for j in 1..785 {
            img[j] = test_data[i].features[j-1];
        }
        //println!("i%args.n_proc={}",i % args.n_proc);
        match write(p2c_pipes[i % args.n_proc].1, &img) {
            Ok(result) => result,
            Err(err) => {
                kill_children(15);
                panic!("Error: {:?}", err);
            }
        };
        match read(c2p_pipes[i % args.n_proc].0, &mut buf) {
            Ok(result) => result,
            Err(err) => {
                kill_children(15);
                panic!("Error: {:?}", err);
            }
        };

        let nearest_index = usize::from_le_bytes(buf[0..8].try_into().expect("Conversion failed in parent"));
        let predicted = buf[8];
        let expected = buf[9];
        //println!("nearest_index: {}, predicted: {}, expected: {}", nearest_index, predicted, expected);
	    if predicted == expected {
	        ok += 1;
	    }
	    else {
	        let digits = b"0123456789";
	        println!("{}[{}] {}[{}]",
		         char::from(digits[predicted as usize]), nearest_index,
		         char::from(digits[expected as usize]), i);
	    }
    }
    println!("{}% success", (ok as f64)/(n as f64)*100.0);

    // reap workers, close open file descriptors
    for i in 0..args.n_proc {
        close(c2p_pipes[i].0).expect("Failed to close file descriptor");
        close(p2c_pipes[i].1).expect("Failed to close file descriptor");
        waitpid(unsafe{CHILD_PIDS[i]}, None).expect("waitpid() failed");
        //println!("Are we hanging here? i = {}", i);
    }
}
