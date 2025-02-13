#ifndef EXIT_H
#define EXIT_H

/* exit() exits only the current thread,
   while exit_group () exits the entire process
 */

void exit (int status);

void exit_group (int status);

#endif
