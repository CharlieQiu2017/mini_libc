#ifndef EXIT_H
#define EXIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* exit() exits only the current thread,
   while exit_group () exits the entire process
 */

__attribute__((noreturn)) void exit (int status);

__attribute__((noreturn)) void exit_group (int status);

#ifdef __cplusplus
}
#endif

#endif
