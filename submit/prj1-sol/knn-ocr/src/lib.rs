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

///rows and columns in image file
const ROW_COL_COUNT: usize = 28;

fn get_u32_from_vec(byte_vector: &Vec<u8>, start: usize) -> u32 {
    u32::from_be_bytes(byte_vector[start..start+4].try_into().unwrap())
}

///return labeled-features with features read from data_dir/data_file_name
///and labels read from data_dir/label_file_name
pub fn read_labeled_data(data_dir: &str, data_file_name: &str, label_file_name: &str) -> Vec<LabeledFeatures> {
    let data_path = format!("{}{}", data_dir, data_file_name);
    let label_path = format!("{}{}", data_dir, label_file_name);
    //println!("{}\n{}", data_path, label_path);

    let data_byte_vector = fs::read(&data_path).expect("Error: invalid data path");
    let label_byte_vector = fs::read(&label_path).expect("Error: invalid label path");
    assert_eq!(get_u32_from_vec(&data_byte_vector, 0), DATA_MAGIC);
    assert_eq!(get_u32_from_vec(&label_byte_vector, 0), LABEL_MAGIC);
    
    let n = get_u32_from_vec(&data_byte_vector, 4) as usize;
    assert_eq!(get_u32_from_vec(&label_byte_vector, 4) as usize, n);

    let n_rows = get_u32_from_vec(&data_byte_vector, 8) as usize;
    assert_eq!(ROW_COL_COUNT , n_rows);

    let n_cols = get_u32_from_vec(&data_byte_vector, 12) as usize;
    assert_eq!(ROW_COL_COUNT, n_cols);

    //println!("n: {}, n_rows: {}, n_cols: {}", n, n_rows, n_cols);
    let mut results: Vec<LabeledFeatures> = Vec::with_capacity(n);
    let mut image_data_start = 16;
    let label_data_start = 8;
    let image_size = 784;
    for i in 0..n {
        let mut x = LabeledFeatures {
            features: Vec::with_capacity(image_size),
            label: label_byte_vector[i + label_data_start]
        };
        x.features.extend_from_slice(&data_byte_vector[image_data_start..image_data_start+image_size]);
        results.push(x);
        image_data_start += image_size;
    }
    results
}

fn cartesian_distance() -> u32 {
    0
}

///Return the index of an image in training_set which is among the k
///nearest neighbors of test and has the same label as the most
///common label among the k nearest neigbors of test.
pub fn knn(training_set: &Vec<LabeledFeatures>, test: &Vec<Feature>, k: usize) -> Index {
    0
}
