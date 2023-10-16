size_t GLOBAL_amount_alloc = 0;
size_t GLOBAL_amount_dealloc = 0;

void *mem_check_alloc(size_t alignment, size_t s) {
  if (alignment < sizeof(void*)) alignment = sizeof(void*);
  size_t *ptr = static_cast<size_t*>(aligned_alloc(alignment, sizeof(size_t)*32+s));
  // we use a buffer of 32*size_t (4/8) to accomodate big alignments up to 128b if needed
  GLOBAL_amount_alloc += s;
  *ptr = s;
  return ptr+32;
}

void mem_check_free(void *raw_ptr) {
  size_t *ptr = static_cast<size_t*>(raw_ptr);
  GLOBAL_amount_dealloc += ptr[-32];
  free(ptr-32);
}

void mem_report() {
  printf("Total memory allocated: %zu\n", GLOBAL_amount_alloc);
  printf("Total memory freed:     %zu\n", GLOBAL_amount_dealloc);
  if (GLOBAL_amount_alloc != GLOBAL_amount_dealloc) abort();
}

