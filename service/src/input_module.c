#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#include <seccomp.h> /* libseccomp */

#include "input_module.h"

void redact_filename(char* filename)
{
   int len = strlen(filename);
   for (int i = 0; i < len; i++)
   {
      if (filename[i] == '.' || filename[i] == '/')
      {
         filename[i] = '_';
      }
   }
}

void run_input_module(queue* preprocessed_executable_packet_queue, queue* processed_executable_packet_queue, int max_shared_fd)
{
   // set up seccomp
   scmp_filter_ctx ctx;
   int rc = 0;

   ctx = seccomp_init(SCMP_ACT_KILL); // default action: kill
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(open), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(openat), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(lseek), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(close), 0);

   if (rc != 0) {
      #ifdef DEBUG
      perror("seccomp_rule_add failed");
      #endif
      return;
   }   

   // load the filter
   seccomp_load(ctx);
   if (rc != 0) {
      #ifdef DEBUG
      perror("seccomp_load failed");
      #endif
      return;
   }


   while(1)
   {
      execution_packet next;
      queue_remove(preprocessed_executable_packet_queue, &next, sizeof(execution_packet));

      switch(next.opcode)
      {
         case OPN: {
            char filename[9];
            int machine_flags = (int)next.data_2;
            strncpy(filename, (char*)&next.data_1, 8);
            filename[8] = '\0';

            redact_filename(filename);
            //printf("%s\n", filename);

            int os_flags = 0;

            if (FLAG_TO_RW_MODE(machine_flags) == FILE_READ_ONLY)
            {
               os_flags |= O_RDONLY;
            }
            else if (FLAG_TO_RW_MODE(machine_flags) == FILE_WRITE_ONLY)
            {
               os_flags |= O_WRONLY;
            }
            else if (FLAG_TO_RW_MODE(machine_flags) == FILE_READ_WRITE)
            {
               os_flags |= O_RDWR;
            }

            if ((machine_flags & FILE_CREATE) != 0 && (next.ring == RING_ZERO))
            {
               os_flags |= O_CREAT;
            }
            if ((machine_flags & FILE_TRUNCATE) != 0)
            {
               os_flags |= O_TRUNC;
            }

            if (strcmp(filename, "gran") == 0 && (next.ring != RING_ZERO))
            {
               next.opcode = DUP;
               next.data_1 = -1;
            }
            else
            {

               int new_fd = open(filename, os_flags, 0600);
               //fprintf(stdout, "Input_module: Opening file %s with os_flags %x new_fd=%d\n", filename, os_flags, new_fd);
               #ifdef DEBUG
               fprintf(stderr, "Input_module: Opening file %s with os_flags %x new_fd=%d\n", filename, os_flags, new_fd);
               #endif
               next.opcode = DUP;
               next.data_1 = new_fd;
            }
            break;
         }

         case RED: {
            int fd = next.data_1;
            char input;

            int result;
            
            // Check the FD, can't read from an FD that's between 2 and max_shared_fd
            if (fd > 2 && fd <= max_shared_fd && next.ring != RING_ZERO)
            {
               #ifdef DEBUG
               fprintf(stderr, "Input_module: Reading from fd %d in ring %d not allowed\n", fd, next.ring);
               #endif
               result = -1;
            }
            else
            {
               result = read(fd, &input, 1);
               //fprintf(stderr, "Input_module: Reading from fd %d got %d with result %d\n", fd, input, result);

               // if file is closed, return -1 (it's what the machine expects)
               if (result == 0)
               {
                  result = -1;
               }
               #ifdef DEBUG
               fprintf(stderr, "Input_module: Reading from fd %d got %c with result %d\n", fd, input, result);
               if (result == -1)
               {
                  perror("read fail");
               }
               #endif
            }
            //fprintf(stdout, "Input_module: Reading from fd %d got %d with result %d\n", fd, input, result);

            next.opcode = DUP;
            if (result == 1)
            {
               next.data_1 = (unsigned char)input;
            }
            else 
            {
               next.data_1 = -1;
            }
            break;
         }

         case WRT: {
            int fd = next.data_1;
            char to_write = next.data_2;
            char result;
            
            // Check the FD, can't write to an FD that's between 2 and max_shared_fd
            if (fd > 2 && fd <= max_shared_fd && next.ring != RING_ZERO)
            {
               //fprintf(stderr, "Input_module: Reading from fd %d in ring %d not allowed\n", fd, next.ring);
               #ifdef DEBUG
               fprintf(stderr, "Input_module: Reading from fd %d in ring %d not allowed\n", fd, next.ring);
               #endif

               result = -1;
            }
            else
            {
               result = write(fd, &to_write, 1);
               //fprintf(stderr, "Input_module: writing to fd %d %d with result %d\n", fd, to_write, result);

               #ifdef DEBUG
               fprintf(stderr, "Input_module: writing to fd %d with result %d\n", fd, result);
               if (result == -1)
               {
                  perror("write fail");
               }
               #endif

            }
            #ifdef DEBUG
            fprintf(stderr, "Input_module: Writing to fd %d a %c with result %d\n", fd, to_write, result);
            #endif

            next.opcode = DUP;
            if (result == 1)
            {
               next.data_1 = TRUE;
            }
            else
            {
               next.data_1 = result;
            }
            break;
         }

         case CLS: {
            int fd = next.data_1;
            int result;
            if (fd > 2 && fd <= max_shared_fd && next.ring != RING_ZERO)
            {
               #ifdef DEBUG
               fprintf(stderr, "Input_module: Closing fd %d in ring %d not allowed\n", fd, next.ring);
               #endif

               result = -1;
            }
            else
            {
               result = close(fd);
            }
            #ifdef DEBUG
            fprintf(stderr, "Input_module: Closing fd %d with result %d\n", fd, result);
            #endif

            next.opcode = DUP;
            if (result == 0)
            {
               next.data_1 = TRUE;
            }
            else
            {
               next.data_1 = result;
            }
            break;
         }
         case LS: {
            int result = system("/bin/ls");
            next.opcode = DUP;
            next.data_1 = result;
            break;
         }
         case SDF: {
            int in_fd = next.data_1;
            int out_fd = next.data_2;
            char input;
            int result;
            
            if ((in_fd > 2 && in_fd <= max_shared_fd && next.ring != RING_ZERO) ||
                (out_fd > 2 && out_fd <= max_shared_fd && next.ring != RING_ZERO))
            {
               #ifdef DEBUG
               fprintf(stderr, "Input_module: sending file from fd %d to fd %d in ring %d not allowed\n", in_fd, out_fd, next.ring);
               #endif

               result = -1;
            }
            else
            {
               result = read(in_fd, &input, 1);
            }
            while (result == 1)
            {
               result = write(out_fd, &input, 1);
               if (result == 1)
               {
                  result = read(in_fd, &input, 1);
               }
            }

            #ifdef DEBUG
            fprintf(stderr, "Input_module: Sendfile from %d to %d resulted in %d\n", in_fd, out_fd, result);
            #endif
            next.opcode = DUP;
            next.data_1 = result;
            break;
         }
         case ULK: {
            char filename[9];
            strncpy(filename, (char*)&next.data_1, 8);
            filename[8] = '\0';
            int result = unlink(filename);

            #ifdef DEBUG
            fprintf(stderr, "Input_module: Unlink file %s resulted in %d\n", filename, result);
            #endif

            next.opcode = DUP;
            next.data_1 = result;
            break;
         }
         case LSK: {
            int fd = next.data_1;
            off_t offset = next.data_2;
            off_t result;
            if (fd > 2 && fd <= max_shared_fd && next.ring != RING_ZERO)
            {
               #ifdef DEBUG
               fprintf(stderr, "Input_module: lseek on fd %d in ring %d not allowed\n", fd, next.ring);
               #endif

               result = -1;
            }
            else
            {
               result = lseek(fd, offset, SEEK_SET);
            }

            #ifdef DEBUG
            fprintf(stderr, "Input_module: lseek fd %d offset %d result %d\n", fd, offset, result);
            #endif

            next.opcode = DUP;
            next.data_1 = result;
            break;
         }

         default:
            break;
      }
      queue_add(processed_executable_packet_queue, &next, sizeof(execution_packet));
   }
}
