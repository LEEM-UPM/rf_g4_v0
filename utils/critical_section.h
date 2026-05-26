#ifndef UTILS_CRITICAL_SECTION_H_
#define UTILS_CRITICAL_SECTION_H_

#ifdef __ARM_ARCH
  #include <cmsis_compiler.h>
  #define CRITICAL_ENTER()  __disable_irq()
  #define CRITICAL_EXIT()   __enable_irq()
#else
  #define CRITICAL_ENTER()  (void)0
  #define CRITICAL_EXIT()   (void)0
#endif

#endif /* UTILS_CRITICAL_SECTION_H_ */