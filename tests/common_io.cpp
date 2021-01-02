#include "spin_mutex.h"

static spin_mutex_t  my_io_mutex; // This is only for serializing parallel prints

#define LOG(...)   do{ spin_mutex_t::scoped_lock l(my_io_mutex); std::cerr << __VA_ARGS__ << std::endl; }while(0)
