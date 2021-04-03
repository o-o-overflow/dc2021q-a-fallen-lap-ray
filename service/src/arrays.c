#include <stdlib.h>
#include <stdio.h>

#include "arrays.h"

#define MAX_ARRAY_SIZE 1024
#define MAX_ARRAYS 100

void** arrays;
data_type* array_size;
uint num_arrays;

void run_array_module(queue* processed_executable_packet_queue, queue* post_array_packet_queue)
{
   arrays = malloc(MAX_ARRAYS*sizeof(void*));
   array_size = malloc(MAX_ARRAYS*sizeof(uint));
   num_arrays = 0;
   
   while(1)
   {
      execution_packet next;
      queue_remove(processed_executable_packet_queue, &next, sizeof(execution_packet));

      switch(next.opcode)
      {
         case NAR:
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
               int current_array_num = num_arrays;
               num_arrays += 1;

               arrays[current_array_num] = malloc(next.data_1);
               array_size[current_array_num] = next.data_1;
               next.opcode = DUP;
               next.data_1 = current_array_num;
            }
            break;
         case ARF:
         {
            data_type array_index = next.data_1;
            data_type array_offset = next.data_2;
            if (array_index >= num_arrays)
            {
               #ifdef DEBUG
               fprintf(stderr, "array_module: Arrays outside %d\n", array_index);
               #endif
               next.opcode = DUP;
               next.data_1 = -1;
               break;
            }
            data_type this_array_size = array_size[array_index];
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
            };

            queue_add(post_array_packet_queue, &ref_packet, sizeof(execution_packet));
            queue_add(post_array_packet_queue, &value_packet, sizeof(execution_packet));
            continue;
         }  
         case AST:
         {
            uint64_t* ref = next.data_1;
            uint64_t value = next.data_2;
            bool safe = 0;

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
            next.opcode = DUP;
            next.data_1 = 1;
            break;
         }
         default:
            break;
      }

      queue_add(post_array_packet_queue, &next, sizeof(execution_packet));
   }
}
