use std::vec::Vec;

/*
 * Loops and Conditionals are not yet allowed in const functions.
 *
const fn expander(n: usize, left: bool) -> u32 {
    let mut data = 0u32;
    let mut val = n as u8;
    for _ in 0..8 {
        let newbit = if left { val >> 7 } else { val & 1 };
        data = (data << 4) | newbit as u32;
        if left {
            val <<= 1;
        } else {
            val >>= 1;
        }
    }
    data
}
*/

const fn expander_left(n: usize) -> u32 {
    let mut data = 0u32;
    let val = n as u32;
    data = (data << 4) | (val >> 7) & 1;
    data = (data << 4) | (val >> 6) & 1;
    data = (data << 4) | (val >> 5) & 1;
    data = (data << 4) | (val >> 4) & 1;
    data = (data << 4) | (val >> 3) & 1;
    data = (data << 4) | (val >> 2) & 1;
    data = (data << 4) | (val >> 1) & 1;
    data = (data << 4) | (val >> 0) & 1;
    data
}

const fn expander_right(n: usize) -> u32 {
    let mut data = 0u32;
    let val = n as u32;
    data = (data << 4) | (val >> 0) & 1;
    data = (data << 4) | (val >> 1) & 1;
    data = (data << 4) | (val >> 2) & 1;
    data = (data << 4) | (val >> 3) & 1;
    data = (data << 4) | (val >> 4) & 1;
    data = (data << 4) | (val >> 5) & 1;
    data = (data << 4) | (val >> 6) & 1;
    data = (data << 4) | (val >> 7) & 1;
    data
}

lazy_static! {
    pub static ref EXPAND_L: Vec<u32> = (0..256).map(expander_left).collect();
    pub static ref EXPAND_R: Vec<u32> = (0..256).map(expander_right).collect();
}
