use std::io::{self, BufRead};
use std::str;
use httparse;
use std::collections::HashMap;


#[derive(Debug)]
pub struct Request {
    pub method: String,
    pub path: String,
    pub headers: HashMap<String, String>,
    pub body: Vec<u8>,
}

pub fn parse_request<R>(mut input: R) -> io::Result<Request>
    where R: BufRead
{    
   let mut buf = Vec::new();

    while let Ok(n) = input.read_until(b'\n', &mut buf) {
	if n == 2 && buf[buf.len() - 2] == b'\r' { break; }
    }
    const MAX_HEADERS: usize = 32;
    let mut headers_array = [httparse::EMPTY_HEADER; MAX_HEADERS];
    let mut req = httparse::Request::new(&mut headers_array);
    req.parse(&buf).unwrap();
    let method = req.method.unwrap().to_string().to_uppercase();
    let path = req.path.unwrap().to_string();
    let mut headers = HashMap::new();
    for header in headers_array {
	let name = header.name.to_string();
	if name.len() > 0 {
	    headers.insert(name.to_uppercase(),
			   str::from_utf8(header.value).unwrap().to_string());
	}
    }
    let len = match headers.get("CONTENT-LENGTH") {
	None => 0,
	Some(val) => val.parse::<usize>().unwrap(),
    };
    let mut body = vec![0; len];
    input.read_exact(&mut body)?;
    
    //for (k, v) in &headers { println!("{}: {}", k, v); }
    Ok(Request { method, path, headers, body  })
}
