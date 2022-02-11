//remove once project is completed
#![allow(dead_code)]
#![allow(unused_variables, unused_imports)]


use std::fs;

//this may result in non-deterministic behavior
//use std::collections::HashMap;

//use this for deterministic behavior
//use hash_hasher::HashedMap;

type Feature = u8;
type Label = u8;
type Index = usize;

pub struct LabeledFeatures {
    ///feature set
    pub features: Vec<Feature>,

    ///classification of feature set
    pub label: Label,
}

///magic number used at start of MNIST data file
const DATA_MAGIC: u32 = 0x803;

///magic number used at start of MNIST label file
const LABEL_MAGIC: u32 = 0x801;

///return labeled-features with features read from data_dir/data_file_name
///and labels read from data_dir/label_file_name

fn get_magic_number(byte_slice: &[u8]) -> u32 {
    /*
    let ret: u32; 
    for v in byte_slice.iterator() {
        ret +
    }
    */
}

pub fn read_labeled_data(data_dir: &str, data_file_name: &str, label_file_name: &str) -> Vec<LabeledFeatures> {
    let results = Vec::new();
    let data_path = format!("{}{}", data_dir, data_file_name);
    let label_path = format!("{}{}", data_dir, label_file_name);
    let data_byte_vector = fs::read(&data_path).expect("Error: invalid data path");
    let label_byte_vector = fs::read(&data_path).expect("Error: invalid label path");
    let x: u32 = get_magic_number(&data_byte_vector[0..3]);
    results
}

///Return the index of an image in training_set which is among the k
///nearest neighbors of test and has the same label as the most
///common label among the k nearest neigbors of test.
pub fn knn(training_set: &Vec<LabeledFeatures>, test: &Vec<Feature>, k: usize) -> Index {
    0  //dummy return value
}
