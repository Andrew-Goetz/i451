use std::path::Path;
use std::env;
use std::error::Error;

use knn_ocr::{read_labeled_data, knn};
use nix::unistd::{fork, ForkResult, pipe, close, Pid, read, write};
use nix::sys::wait::waitpid;
use std::os::unix::prelude::RawFd;

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

fn if_error(pids: Vec<Pid>) {

}

fn do_child(read_img: RawFd, write_result: RawFd) {
    ()
}

fn main() {
    let argv: Vec<String> = env::args().collect();
    let args;
    match Args::new(&argv) {
    	Err(err) => usage(&argv[0], &err.to_string()),
    	Ok(a) => args = a,
    };

    let train_data = read_labeled_data(&args.data_dir, TRAINING_DATA, TRAINING_LABELS);
    let test_data = read_labeled_data(&args.data_dir, TEST_DATA, TEST_LABELS);
    let n: usize = if args.n_test <= 0 {
	    test_data.len()
    } else {
	    args.n_test as usize
    };

    
    // Spawn worker processes and their pipes, printing out (and recording) PIDs
    let mut child_pids: Vec<Pid> = Vec::new();
    let mut p2c_pipes: Vec<(RawFd, RawFd)> = Vec::new();
    let mut c2p_pipes: Vec<(RawFd, RawFd)> = Vec::new();
    for i in 0..args.n_proc {
        p2c_pipes.push(pipe().expect("Failed to create pipe"));
        c2p_pipes.push(pipe().expect("Failed to create pipe"));
        match unsafe{fork()} {
            Ok(ForkResult::Parent{child}) => {
                println!("Process with PID {} created", child);
                child_pids.push(child);
                close(p2c_pipes[i].0).expect("Failed to close file descriptor");
                close(c2p_pipes[i].1).expect("Failed to close file descriptor");
            }
            Ok(ForkResult::Child) => {
                for j in 0..(i-1) {
                    close(p2c_pipes[i].0).expect("Failed to close file descriptor");
                    close(p2c_pipes[i].1).expect("Failed to close file descriptor");
                    close(c2p_pipes[i].0).expect("Failed to close file descriptor");
                    close(c2p_pipes[i].1).expect("Failed to close file descriptor");
                }
                close(p2c_pipes[i].1).expect("Failed to close file descriptor");
                close(c2p_pipes[i].0).expect("Failed to close file descriptor");
                do_child(p2c_pipes[i].0, c2p_pipes[i].1);
            }
            Err(_) => {
                panic!("Fork failed");
            }
        }
    }


    let mut ok = 0;
    for i in 0..n {
	    let nearest_index = knn(&train_data, &test_data[i].features, args.k);
	    let predicted = train_data[nearest_index].label;
	    let expected = test_data[i].label;
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

    // wait workers, close pipes
    for i in 0..args.n_proc {
        waitpid(child_pids[i], None).expect("waitpid() failed");
        close(p2c_pipes[i].1).expect("Failed to close file descriptor");
        close(c2p_pipes[i].0).expect("Failed to close file descriptor");
    }
    println!("{}% success", (ok as f64)/(n as f64)*100.0);
}
