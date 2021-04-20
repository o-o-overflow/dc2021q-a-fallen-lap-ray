#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/random.h>
#include <time.h>
#include <unistd.h>

#include "processing_unit.h"
#include "queue.h"
#include "khash.h"


uint64_t hash(uint64_t var)
{
   return kh_int64_hash_func(var);
}

tag_area_type new_tag_area()
{
   return (tag_area_type) random();
}


void send_result(execution_result result, queue* outgoing_token_packets)
{   
   // Always add the first output
   queue_add(outgoing_token_packets, &result.output_1, sizeof(token_type));

   if (result.marker == BOTH_OUTPUT_MARKER)
   {
      queue_add(outgoing_token_packets, &result.output_2, sizeof(token_type));
   }
}

void run_processing_unit(queue* incoming_execution_packets, queue* outgoing_token_packets)
{
   #ifdef DEBUG
   pid_t pid = getpid();
   #endif

   // Init the random seed
   unsigned int seed;
   getrandom(&seed, sizeof(int), 0);
   srandom(seed);

   while(1)
   {
	  execution_packet next;
	  execution_result result;
	  queue_remove(incoming_execution_packets, &next, sizeof(execution_packet));

	  result = function_unit(next);

      #ifdef DEBUG
      fprintf(stderr, "%d: Execution result of %d %s %lu %lu 0x%lx\n",
              pid,
              DESTINATION_TO_ADDRESS(next.input),
              opcode_to_name[next.opcode],
              next.data_1,
              next.data_2,
              next.tag);
      print_result(result);
      #endif

      send_result(result, outgoing_token_packets);

   }
}

execution_result function_unit(execution_packet packet)
{
   data_type computation_result;
   execution_result output;
   output.marker = ONE_OUTPUT_MARKER;

   // Default is that the output tags are the same as the input tag
   output.output_1.tag = packet.tag;
   output.output_2.tag = packet.tag;
   
   switch(packet.opcode)
   {
	  // data_1 + data_2
	  case ADD:
		 computation_result = (data_type)((int64_t)packet.data_1 + (int64_t)packet.data_2);
		 break;
	  // data_1 - data_2
	  case SUB: 
		 computation_result = (data_type)((int64_t)packet.data_1 - (int64_t)packet.data_2);
		 break;
	  // data_1 < data_2
	  case LT:
		 if ((int64_t)packet.data_1 < (int64_t)packet.data_2)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;

	  // data_1 > data_2
	  case GT:
		 if ((int64_t)packet.data_1 > (int64_t)packet.data_2)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;

	  // data_1 >= data_2
	  case GTE:
		 if ((int64_t)packet.data_1 >= (int64_t)packet.data_2)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;

	  // data_1 <= data_2
	  case LTE:
		 if ((int64_t)packet.data_1 <= (int64_t)packet.data_2)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;
		 
	  // data_1 == data_2
	  case EQ:
		 if ((int64_t)packet.data_1 == (int64_t)packet.data_2)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;

	  // data_1 != data_2
	  case NEQ:
		 if ((int64_t)packet.data_1 == (int64_t)packet.data_2)
		 {
			computation_result = FALSE;
		 }
		 else
		 {
			computation_result = TRUE;
		 }
		 break;

	  // !data_1
	  case NEG:
		 if (packet.data_1 == FALSE)
		 {
			computation_result = TRUE;
		 }
		 else
		 {
			computation_result = FALSE;
		 }
		 break;
	  // duplication data_1 (data_2 ignored)
	  case DUP:
		 computation_result = packet.data_1;
		 break;

	  case BRR:
		 
		 /* BRR semantics:
			if (data_2)
			{
			  destination_1 = data_1;
			}
			else
			{
			  destination_2 = data_1;
			}
		 */
		 if (packet.data_2 != FALSE)
		 {
			output.output_1.destination = packet.destination_1;
			output.output_1.data = packet.data_1;
		 }
		 else
		 {
			output.output_1.destination = packet.destination_2;
			output.output_1.data = packet.data_1;
		 }
		 break;


	  // destination_1 = data_1
	  // The key idea here is that MERge takes two inputs and outputs the first that is ready.
	  // The matching_store will take care of making sure that whatever is ready is in the first packet.
	  case MER:
		 computation_result = packet.data_1;
		 break;

	  // data_1 = new_tag_area(), iteration_count = 0
	  // data_2 = data_1.tag
	  case NTG:
		 output.output_1.data = CREATE_TAG(new_tag_area(), 0);
		 output.output_1.destination = packet.destination_1;
		 output.output_1.tag = packet.tag;

		 output.output_2.destination = packet.destination_2;
		 output.output_2.data = packet.tag;
		 output.output_2.tag = packet.tag;

		 output.marker = BOTH_OUTPUT_MARKER;
		 break;

	  // output.tag = data_1.tag + 1
	  case ITG:
		 computation_result = packet.data_1;
		 output.output_1.tag = packet.tag + 1;
		 output.output_2.tag = output.output_1.tag;
		 break;

	  // output = data_1
	  // output.tag.iteration_count = data_2
	  case SIL:
		 computation_result = packet.data_1;
		 output.output_1.tag = CREATE_TAG(TAG_TO_TAG_AREA(packet.tag), (iteration_count_type)packet.data_2);
		 output.output_2.tag = output.output_1.tag;
		 break;

	  // output = data_2
	  // output.tag = data_1
	  case CTG:
		 computation_result = packet.data_2;
		 output.output_1.tag = packet.data_1;
		 output.output_2.tag = packet.data_1;
		 break;

	  // output = data_1
	  // destination_1 = data_2
	  case RTD:
		 output.output_1.data = packet.data_1;
		 output.output_1.destination = packet.data_2;
		 break;

	  // output = data_1.tag
	  case ETG:
		 computation_result = packet.tag;
		 break;

	  // output = data_1 * data_2
	  case MUL:
		 computation_result = packet.data_1 * packet.data_2;
		 break;

	  // output = data_1 % data_2
	  case MOD:
		 computation_result = packet.data_1 % packet.data_2;
		 break;


	  // output = data_1 ^ data_2
	  case XOR:
		 computation_result = packet.data_1 ^ packet.data_2;
		 break;

	  // output = data_1 & data_2
	  case AND:
		 computation_result = packet.data_1 & packet.data_2;
		 break;

	  // output = data_1 | data_2
	  case OR:
		 computation_result = packet.data_1 | packet.data_2;
		 break;

	  // output = data_1 << data_2
	  case SHL:
		 computation_result = packet.data_1 << packet.data_2;
		 break;

	  // output = data_2 >> data_1
	  case SHR:
		 computation_result = packet.data_1 >> packet.data_2;
		 break;

	  // Shutdown the system: exit(data_1);
	  case HLT:
		 exit(packet.data_1);
		 break;

      // return random()
      case RND:
         computation_result = (data_type) (random() ^ packet.tag ^ time(NULL));
         break;

	  // we should never get these instructions, they should be handled by other modules
	  case OPN:
	  case RED:
	  case WRT:
	  case CLS:
	  case LOD:
	  case LS:
      case SDF:
      case ULK:
      case LSK:
      case NAR:
      case ARF:
      case AST:
		 #ifdef DEBUG
		 assert(false);
		 #endif
		 break;
   }

   if (packet.opcode != BRR &&
	   packet.opcode != NTG &&
	   packet.opcode != RTD)
   {
	  /* pure computation, put the output correctly */
	  output.output_1.destination = packet.destination_1;
	  output.output_1.data =  computation_result;

	  if (packet.marker == BOTH_OUTPUT_MARKER)
	  {
		 output.output_2.destination = packet.destination_2;
		 output.output_2.data = computation_result;
		 output.marker = BOTH_OUTPUT_MARKER;
	  }
   }
   return output;
}
