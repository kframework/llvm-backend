extern crate libc;

use super::decls::{List,Int,K,KElem,__gmpz_fits_ulong_p,__gmpz_fits_slong_p,__gmpz_get_si,__gmpz_get_ui,__gmpz_init_set_ui,move_int,printConfigurationInternal};
use std::ptr;
use std::mem;
use std::cmp::Ordering;
use std::hash::Hash;
use std::collections::hash_map::DefaultHasher;
use std::ffi::CString;
use self::libc::{FILE,c_char,c_void,fprintf};

#[no_mangle]
pub extern "C" fn size_list() -> usize {
  mem::size_of::<List>()
}

#[no_mangle]
pub unsafe extern "C" fn drop_list(ptr: *mut List) {
  ptr::drop_in_place(ptr)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_unit() -> List {
  List::new()
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_cmp(a: *const c_void, b: *const c_void) -> i64 {
  match std::mem::transmute::<*const c_void, &List>(a)
      .cmp(std::mem::transmute::<*const c_void, &List>(b)) {
    Ordering::Less => -1,
    Ordering::Equal => 0,
    Ordering::Greater => 1,
  }
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_element(value: K) -> List {
  List::singleton(KElem::new(value))
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_concat(l1: *const List, l2: *const List) -> List {
  let mut tmp = (*l1).clone();
  tmp.append((*l2).clone());
  tmp
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_in(value: K, list: *const List) -> bool {
  (*list).contains(&KElem::new(value))
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_in_keys(index: *const Int, list: *const List) -> bool {
  let (status, index_long) = get_long(index);
  if !status {
    panic!("Index out of range")
  }
  index_long < (*list).len()
}

unsafe fn get_long(i: *const Int) -> (bool, usize) {
  if !(__gmpz_fits_ulong_p(i) != 0) {
    return (false, 0);
  }
  (true, __gmpz_get_ui(i))
}

unsafe fn get_slong(i: *const Int) -> (bool, isize) {
  if !(__gmpz_fits_slong_p(i) != 0) {
    return (false, 0);
  }
  (true, __gmpz_get_si(i))
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_get(list: *const List, index: *const Int) -> K {
  let (status, index_long) = get_slong(index);
  if !status {
    panic!("Index out of range")
  }
  hook_LIST_get_long(list, index_long)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_get_long(list: *const List, index: isize) -> K {
  let index_long = if index < 0 { ((*list).len() as isize) + index } else { index } as usize;
  match (*list).get(index_long) {
    Some(KElem(elem)) => { *elem.get() }
    None => panic!("Index out of range")
  }
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_lookup(list: *const List, index: *const Int) -> K {
  hook_LIST_get(list, index)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_range(list: *const List, from_front: *const Int, from_back: *const Int) -> List {
  let (status, front_long) = get_long(from_front);
  if !status {
    panic!("Index out of range")
  }
  let (status, back_long) = get_long(from_back);
  if !status {
    panic!("Index out of range")
  }
  hook_LIST_range_long(list, front_long, back_long)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_range_long(list: *const List, from_front: usize, from_back: usize) -> List {
  let old_len = (*list).len();
  if old_len < from_front + from_back {
    panic!("Index out of range")
  }
  (*list).clone().slice(from_front..old_len - from_back)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_size(l: *const List) -> *mut Int {
  let mut result = Int(0, 0, ptr::null());
  __gmpz_init_set_ui(&mut result, hook_LIST_size_long(l));
  move_int(&mut result)
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_size_long(l: *const List) -> usize {
  (*l).len()
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_make(len: *const Int, value: K) -> List {
  let mut tmp = List::new();
  let (status, len_long) = get_long(len);
  if !status {
    panic!("Index out of range")
  }
  for _ in 0..len_long {
    tmp.push_back(KElem::new(value));
  }
  tmp
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_update(list: *const List, index: *const Int, value: K) -> List {
  let (status, index_long) = get_long(index);
  if !status {
    panic!("Index out of range")
  }
  if index_long >= (*list).len() {
    panic!("Index out of range")
  }
  (*list).update(index_long, KElem::new(value))
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_updateAll(l1: *const List, index: *const Int, l2: *const List) -> List {
  let (status, index_long) = get_long(index);
  if !status {
    panic!("Index out of range")
  }
  if index_long != 0 && (*l2).len() != 0 {
    if index_long + (*l2).len() - 1 >= (*l1).len() {
      panic!("Index out of range")
    }
  }
  let mut before = (*l1).take(index_long);
  let after = (*l1).skip(index_long + (*l2).len());
  before.append((*l2).clone());
  before.append(after);
  before
}

#[no_mangle]
pub unsafe extern "C" fn hook_LIST_eq(l1: *const List, l2: *const List) -> bool {
  *l1 == *l2
}

#[no_mangle]
pub unsafe extern "C" fn list_hash(l: *const List, h: *mut c_void) {
  let hasher = h as *mut &mut DefaultHasher;
  l.hash(*hasher)
}

#[no_mangle]
pub unsafe extern "C" fn printList(file: *mut FILE, list: *const List, unit: *const c_char, element: *const c_char, concat: *const c_char) {
  if (*list).len() == 0 {
    let fmt = CString::new("%s()").unwrap();
    fprintf(file, fmt.as_ptr(), unit);
    return;
  }
  let mut i = 1;
  let parens = CString::new(")").unwrap();
  let comma = CString::new(",").unwrap();
  let sort = CString::new("K").unwrap();
  let fmt = CString::new("%s(").unwrap();
  for KElem(value) in (*list).iter() {
    if i < (*list).len() {
      fprintf(file, fmt.as_ptr(), concat);
    }
    fprintf(file, fmt.as_ptr(), element);
    printConfigurationInternal(file, *value.get(), sort.as_ptr());
    fprintf(file, parens.as_ptr());
    if i < (*list).len() {
      fprintf(file, comma.as_ptr());
    }
    i += 1
  }
  for _ in 0..(*list).len()-1 {
    fprintf(file, parens.as_ptr());
  }
}

#[no_mangle]
pub unsafe extern "C" fn list_foreach(list: *mut List, process: extern fn(block: *mut K)) {
  for value in (*list).iter() {
    process(value.0.get());
  }
}

#[cfg(test)]
pub mod tests {
  extern crate libc;

  use decls::testing::*;
  use hook_list::*;

  #[test]
  fn test_element() {
    unsafe {
      let list = hook_LIST_element(DUMMY0);
      let index = alloc_int();
      __gmpz_init_set_ui(index, 0);
      let result = hook_LIST_get(&list, index);
      assert_eq!(result, DUMMY0);
      let index2 = alloc_int();
      __gmpz_init_set_si(index2, -1);
      let result = hook_LIST_get(&list, index2);
      assert_eq!(result, DUMMY0);
      free_int(index);
      free_int(index2);
    }
  }

  #[test]
  fn test_unit() {
    unsafe {
      let list = hook_LIST_unit();
      let result = hook_LIST_size(&list);
      assert_eq!(__gmpz_cmp_ui(result, 0), 0);
      free_int(result);
    }
  }

  #[test]
  fn test_concat() {
    unsafe {
      let l1 = hook_LIST_element(DUMMY0);
      let l2 = hook_LIST_element(DUMMY1);
      let list = hook_LIST_concat(&l1, &l2);
      let index = alloc_int();
      __gmpz_init_set_ui(index, 0);
      let result = hook_LIST_get(&list, index);
      assert_eq!(result, DUMMY0);
      __gmpz_clear(index);
      __gmpz_init_set_ui(index, 1);
      let result = hook_LIST_get(&list, index);
      assert_eq!(result, DUMMY1);
      __gmpz_clear(index);
      let index = hook_LIST_size(&list);
      assert_eq!(__gmpz_cmp_ui(index, 2), 0);
      free_int(index);
    }
  }

  #[test]
  fn test_in() {
    unsafe {
      let list = hook_LIST_element(DUMMY0);
      let result = hook_LIST_in(DUMMY0, &list);
      assert!(result);
      let result = hook_LIST_in(DUMMY1, &list);
      assert!(!result);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_get_negative() {
    unsafe {
      let index = alloc_int();
      __gmpz_init_set_si(index, -2);
      let list = hook_LIST_element(DUMMY0);
      hook_LIST_get(&list, index);
    }
  }
  
  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_get_out_of_range() {
    unsafe {
      let index = alloc_int();
      __gmpz_init_set_ui(index, 1);
      let list = hook_LIST_element(DUMMY0);
      hook_LIST_get(&list, index);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_range_neg_idx() {
    unsafe {
      let neg = alloc_int();
      let zero = alloc_int();
      __gmpz_init_set_si(neg, -1);
      __gmpz_init_set_ui(zero, 0);
      let list = hook_LIST_element(DUMMY0);
      hook_LIST_range(&list, neg, zero);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_range_neg_len() {
    unsafe {
      let neg = alloc_int();
      let zero = alloc_int();
      __gmpz_init_set_si(neg, -1);
      __gmpz_init_set_ui(zero, 0);
      let list = hook_LIST_element(DUMMY0);
      hook_LIST_range(&list, zero, neg);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_range_out_of_range() {
    unsafe {
      let one = alloc_int();
      __gmpz_init_set_ui(one, 1);
      let list = hook_LIST_element(DUMMY0);
      hook_LIST_range(&list, one, one);
    }
  }

  #[test]
  fn test_range() {
    unsafe {
      let zero = alloc_int();
      let one = alloc_int();
      __gmpz_init_set_ui(zero, 0);
      __gmpz_init_set_ui(one, 1);
      let list = hook_LIST_element(DUMMY0);
      let result = hook_LIST_range(&list, zero, one);
      free_int(zero);
      free_int(one);
      let zero = hook_LIST_size(&result);
      assert_eq!(__gmpz_cmp_ui(zero, 0), 0);
      free_int(zero);
      let zero = alloc_int();
      let one = alloc_int();
      __gmpz_init_set_ui(zero, 0);
      __gmpz_init_set_ui(one, 1);
      let list = hook_LIST_concat(&list, &list);
      let result = hook_LIST_range(&list, one, zero);
      let one = hook_LIST_size(&result);
      assert_eq!(__gmpz_cmp_ui(one, 1), 0);
      free_int(zero);
      free_int(one);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_make_out_of_range() {
    unsafe {
      let neg = alloc_int();
      __gmpz_init_set_si(neg, -1);
      hook_LIST_make(neg, DUMMY0);
    }
  }

  #[test]
  fn test_make() {
    unsafe {
      let zero = alloc_int();
      let ten = alloc_int();
      __gmpz_init_set_ui(zero, 0);
      __gmpz_init_set_ui(ten, 10);
      let list = hook_LIST_make(ten, DUMMY0);
      free_int(ten);
      let result = hook_LIST_get(&list, zero);
      assert_eq!(result, DUMMY0);
      let ten = hook_LIST_size(&list);
      assert_eq!(__gmpz_cmp_ui(ten, 10), 0);
      free_int(zero);
      free_int(ten);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_update_neg() {
    unsafe {
      let list = hook_LIST_element(DUMMY0);
      let neg = alloc_int();
      __gmpz_init_set_si(neg, -1);
      hook_LIST_update(&list, neg, DUMMY1);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_update_out_of_range() {
    unsafe {
      let list = hook_LIST_element(DUMMY0);
      let one = alloc_int();
      __gmpz_init_set_ui(one, 1);
      hook_LIST_update(&list, one, DUMMY1);
    }
  }

  #[test]
  fn test_update() {
    unsafe {
      let list = hook_LIST_element(DUMMY0);
      let index = alloc_int();
      __gmpz_init_set_ui(index, 0);
      let list = hook_LIST_update(&list, index, DUMMY1);
      let result = hook_LIST_get(&list, index);
      assert_eq!(result, DUMMY1);
      free_int(index);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_update_all_neg() {
    unsafe {
      let neg = alloc_int();
      __gmpz_init_set_si(neg, -1);
      let l1 = hook_LIST_element(DUMMY0);
      let l2 = hook_LIST_unit();
      hook_LIST_updateAll(&l1, neg, &l2);
    }
  }

  #[test]
  fn test_update_all() {
    unsafe {
      let zero = alloc_int();
      let one = alloc_int();
      __gmpz_init_set_ui(zero, 0);
      __gmpz_init_set_ui(one, 1);
      let l1 = hook_LIST_element(DUMMY0);
      let l2 = hook_LIST_unit();
      let list = hook_LIST_updateAll(&l1, one, &l2);
      let result = hook_LIST_get(&list, zero);
      assert_eq!(result, DUMMY0);
      let list = hook_LIST_updateAll(&l1, zero, &l2);
      let result = hook_LIST_get(&list, zero);
      assert_eq!(result, DUMMY0);
      let l2 = hook_LIST_element(DUMMY1);
      let list = hook_LIST_updateAll(&l1, zero, &l2);
      let result = hook_LIST_get(&list, zero);
      assert_eq!(result, DUMMY1);
      free_int(zero);
      free_int(one);
    }
  }

  #[test]
  #[should_panic(expected = "Index out of range")]
  fn test_update_all_out_of_range() {
    unsafe {
      let one = alloc_int();
      __gmpz_init_set_ui(one, 1);
      let l1 = hook_LIST_element(DUMMY0);
      let l2 = hook_LIST_element(DUMMY1);
      hook_LIST_updateAll(&l1, one, &l2);
    }
  }

  #[test]
  fn test_eq() {
    unsafe {
      let l1 = hook_LIST_element(DUMMY0);
      let l2 = hook_LIST_unit();
      let result = hook_LIST_eq(&l1, &l2);
      assert!(!result);
      let l2 = hook_LIST_element(DUMMY0);
      let result = hook_LIST_eq(&l1, &l2);
      assert!(result);
    }
  }
}
