#![no_std]
#![no_main]
#![feature(alloc_error_handler)]

extern crate alloc;

//Deze twee regels zijn voor nu noodzakelijk
//anders vind de linker de interrupt vectors niet
#[allow(unused_imports)]
use stm32f4::stm32f411::{interrupt, Interrupt, NVIC};

use alloc::vec::Vec;
use alloc::boxed::Box;
use alloc_cortex_m::CortexMHeap;
use core::alloc::Layout;
use core::panic::PanicInfo;
use cortex_m_rt::entry;

#[global_allocator]
static ALLOCATOR: CortexMHeap = CortexMHeap::empty();

#[entry]
fn main() -> ! {
    // Initialize the allocator BEFORE you use it
    {
        use core::mem::MaybeUninit;
        const HEAP_SIZE: usize = 1024;
        static mut HEAP: [MaybeUninit<u8>; HEAP_SIZE] = [MaybeUninit::uninit(); HEAP_SIZE];
        unsafe { ALLOCATOR.init(HEAP.as_ptr() as usize, HEAP_SIZE) }
    }


    let mut xs: Vec<Box<u8>> = Vec::new(); // box places the value on the heap instead of the stack
    xs.push(Box::new(1));

    loop { /* .. */ }
}

#[alloc_error_handler]
fn oom(_: Layout) -> ! {
    loop {}
}

#[panic_handler]
fn panic(_: &PanicInfo) -> ! {
    loop {}
}