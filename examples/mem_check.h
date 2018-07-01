size_t GLOBAL_amount_alloc = 0;
size_t GLOBAL_amount_dealloc = 0;

void *mem_check_alloc(size_t s) {
  size_t *ptr = static_cast<size_t*>(malloc(sizeof(size_t)+s));
  GLOBAL_amount_alloc += s;
  *ptr = s;
  return ptr+1;
}

void mem_check_free(void *raw_ptr) {
  size_t *ptr = static_cast<size_t*>(raw_ptr);
  GLOBAL_amount_dealloc += ptr[-1];
  free(ptr-1);
}

void mem_report() {
  printf("Total memory allocated: %zu\n", GLOBAL_amount_alloc);
  printf("Total memory freed:     %zu\n", GLOBAL_amount_dealloc);
  if (GLOBAL_amount_alloc != GLOBAL_amount_dealloc) abort();
}

