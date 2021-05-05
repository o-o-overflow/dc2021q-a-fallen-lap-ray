#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#include <seccomp.h> /* libseccomp */

#include "arrays.h"

#define MAX_ARRAY_SIZE 4096
#define MAX_ARRAYS 100

void** arrays;
data_type* array_size;
uint num_arrays;

void run_array_module(queue* processed_executable_packet_queue, queue* post_array_packet_queue)
{
   // set up seccomp
   scmp_filter_ctx ctx;
   int rc = 0;

   ctx = seccomp_init(SCMP_ACT_KILL); // default action: kill
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(brk), 0);

   //rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);


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


   arrays = malloc(MAX_ARRAYS*sizeof(void*));
   array_size = malloc(MAX_ARRAYS*sizeof(uint));
   num_arrays = 0;
   
   while(1)
   {
      execution_packet next;
      queue_remove(processed_executable_packet_queue, &next, sizeof(execution_packet));
      #ifdef DEBUG
      assert(next.ring == RING_ONE);
      #endif

      switch(next.opcode)
      {
         case NAR:
            next.opcode = DUP;
            if (num_arrays == MAX_ARRAYS)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Too many arrays %d\n", num_arrays);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
            }
            else if (next.data_1 > MAX_ARRAY_SIZE)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Requested array too large %d\n", next.data_1);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
            }
            else if (next.data_1 <= 0)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Requested array too small %d\n", next.data_1);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
            }

            else
            {
               if (num_arrays == 0)
               {
                  //printf("next: %p next.ring: %p\n", &next, &next.ring);
               }
               int current_array_num = num_arrays;
               num_arrays += 1;

               arrays[current_array_num] = malloc(next.data_1);
               //fprintf(stderr, "%p\n", arrays[current_array_num]);
               array_size[current_array_num] = next.data_1;
               next.data_1 = current_array_num;
            }
            break;
         case ARF:
         {
            data_type array_index = next.data_1;
            // should be able to make a negative offset
            int64_t array_offset = (int64_t) next.data_2;
            if (array_index >= num_arrays)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Arrays outside %d\n", array_index);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
               break;
            }
            int64_t this_array_size = (int64_t)array_size[array_index];
            //printf("%d %d %d %d\n", array_index, array_offset, this_array_size, array_offset >= this_array_size);

            if (array_offset >= this_array_size)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Array offset %d\n", array_offset);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
               break;
            }

            char* array = (char*)arrays[array_index];
            char* the_ref = array+next.data_2;
            uint64_t value = *((uint64_t*)the_ref);
            uint64_t ref = (uint64_t)(the_ref);

            execution_packet ref_packet = {
               .data_1 = ref,
               .data_2 = 0,
               .opcode = DUP,
               .tag = next.tag,
               .destination_1 = next.destination_1,
               .destination_2 = 0,
               .input = 0,
               .marker = ONE_OUTPUT_MARKER,
               .ring = RING_ONE,
            };

            execution_packet value_packet = {
               .data_1 = value,
               .data_2 = 0,
               .opcode = DUP,
               .tag = next.tag,
               .destination_1 = next.destination_2,
               .destination_2 = 0,
               .input = 0,
               .marker = ONE_OUTPUT_MARKER,
               .ring = RING_ONE,
            };

            queue_add(post_array_packet_queue, &ref_packet, sizeof(execution_packet));
            queue_add(post_array_packet_queue, &value_packet, sizeof(execution_packet));
            continue;
         }  
         case AST:
         {
            next.opcode = DUP;
            uint64_t* ref = next.data_1;
            uint64_t value = next.data_2;
            bool safe = 1;

            for (int i = 0; i < num_arrays; i++)
            {
               char* array = (char*)arrays[i];
               data_type size = array_size[i];

               if (ref >= array && ref < (array+size))
               {
                  safe = 1;
                  break;
               }
            }

            if (!safe)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: AST outside %d\n", ref);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
               break;
            }

            *ref = value;
            //printf("%p: %d\n", ref, value);
            //printf("%p %hhx %hhx %d\n", &next.ring, next.ring, next.opcode, next.data_1);
            break;
         }
         default:
            break;
      }

      queue_add(post_array_packet_queue, &next, sizeof(execution_packet));
   }
}
