use std::env;
use std::io::{self, Read, Write, BufReader};
use std::error::Error;
use std::net::{TcpListener, TcpStream};
use std::path::Path;
use std::str;
use std::sync::Arc;
use std::thread;
use std::fs;

mod request;
use request::{Request, parse_request};

use knn_ocr::{LabeledFeatures, read_labeled_data, knn};

const TRAINING_DATA: &str  = "train-images-idx3-ubyte";
const TRAINING_LABELS: &str  = "train-labels-idx1-ubyte";

fn print_request(request: Request) {
    println!("request.method: {}", request.method);
    println!("request.path: {}", request.path);
    println!("*****HEADERS*****");
    for val in request.headers.values() {
        println!("   {}", val);
    }
    println!("Body size: {}", request.body.len());
}

fn handle_request(train_data: &Vec<LabeledFeatures>, conn: &mut TcpStream, request: Request, args: &Arc<Args>) {
    if request.method == "GET" && request.path == "/" {
        let contents = fs::read_to_string(&args.index_path).expect("Failed to read file.");
        let message = format!("HTTP/1.0 200 OK\r\nContent-Type: txt/html\r\nContent-Length: {}\r\n\r\n{}", contents.len(), contents);
        writeln!(conn, "{}", contents).expect("Write failed.");
    } else if request.method == "POST" && request.path == "/ocr" {
        unimplemented!();
    } else {
        write!(conn, "HTTP/1.0 404 NOT FOUND\r\n\r\n").unwrap();
    }
}

fn do_server(train_data: Vec<LabeledFeatures>, args: Arc<Args>) {
    let addr = format!("127.0.0.1:{}", args.port);
    let conn = TcpListener::bind(&addr).unwrap();
    loop {
	    match conn.accept() {
	        Ok((client, addr)) => {
	    	    println!("new connection: {:?}", addr);
                let mut client_clone = client.try_clone().expect("TcpStream clone failed.");
                let buf_read = BufReader::new(client);
                let request = parse_request(buf_read).unwrap();
                //print_request(request);
                handle_request(&train_data, &mut client_clone, request, &args);
	        },
	        Err(e) => eprintln!("connection error: {:?}", e),
	    }
    }
}

fn main() {
    let argv: Vec<String> = env::args().collect();
    let args;
    match Args::new(&argv) {
	    Err(err) => usage(&argv[0], &err.to_string()),
	    Ok(a) => args = Arc::new(a),
    };
    /* Read in training images */
    let train_data = read_labeled_data(&args.data_dir, TRAINING_DATA, TRAINING_LABELS);
    assert!(train_data.len() == 60000);
    /* Enter server loop */
    do_server(train_data, args);
}

/*********************** Command-Line Arguments ************************/

fn usage(prog_path: &str, err: &str) -> ! {
    let prog_name =
	Path::new(prog_path).file_name().unwrap().to_str().unwrap();
    eprintln!("{}\nusage: {} PORT DATA_DIR ROOT_HTML_PATH [K]", err, prog_name);
    std::process::exit(1);
}

#[derive(Debug)]
struct Args {
    port: u16,
    data_dir: String,
    index_path: String,
    k: usize,
}

impl Args {
    fn new(argv: &Vec<String>) -> Result<Args, Box<dyn Error>> {
	let argc = argv.len();
	if argc < 4 || argc > 5 {
	    Err("must be called with 3 ... 4 args")?;
	}
	let port = argv[1].parse::<u16>()?;
	if port < 1024 	{
	    Err("port must be greater than 1023")?;
	}
	let data_dir = argv[2].clone();
	let index_path = argv[3].clone();
	let mut k: usize = 3;
	if argc == 5 {
	    k = argv[4].parse::<usize>()?;
	    if k == 0 {
		Err("k must be positive")?;
	    }
	}
	Ok(Args { port, data_dir, index_path, k })
    }
    
}
