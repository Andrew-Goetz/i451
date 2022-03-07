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

    // Create pipes before worker processes are created
    let parent_to_child = pipe().unwrap();
    let child_to_parent = pipe().unwrap();
    let (child_read_img, write_img) = (parent_to_child.0, parent_to_child.1);
    let (read_result, child_write_result) = (child_to_parent.0, child_to_parent.1);

    // Spawn worker processes, printing out (and recording) PIDs
    let mut child_pids: Vec<Pid> = Vec::new();
    for _i in 0..args.n_proc {
        match unsafe{fork()} {
            Ok(ForkResult::Parent{child}) => {
                println!("Process with PID {} created", child);
                child_pids.push(child);
            }
            Ok(ForkResult::Child) => {
                close(write_img).unwrap();
                close(read_result).unwrap();
                do_child(child_read_img, child_write_result);
            }
            Err(_) => {
                panic!("Fork failed");
            }
        }
    }
    close(child_read_img).unwrap();
    close(child_write_result).unwrap();

    let train_data = read_labeled_data(&args.data_dir, TRAINING_DATA, TRAINING_LABELS);
    let test_data = read_labeled_data(&args.data_dir, TEST_DATA, TEST_LABELS);
    let n: usize = if args.n_test <= 0 {
	    test_data.len()
    } else {
	    args.n_test as usize
    };


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
    for i in 0..child_pids.len() {
        waitpid(child_pids[i], None).expect("waitpid() failed");
    }
    println!("{}% success", (ok as f64)/(n as f64)*100.0);
    close(write_img).unwrap();
    close(read_result).unwrap();
}
