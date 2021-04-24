#include <stdio.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>

#include <seccomp.h> /* libseccomp */


#include "io_switch.h"

void run_io_switch(queue* execution_token_output_queue, queue* matching_unit_input_queue)
{
   // set up seccomp
   scmp_filter_ctx ctx;
   int rc = 0;

   ctx = seccomp_init(SCMP_ACT_KILL); // default action: kill
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(rt_sigreturn), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(futex), 0);
   rc += seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);

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

   
   token_type next_token;

   while (1)
   {
	  queue_remove(execution_token_output_queue, &next_token, sizeof(token_type));

	  switch(next_token.destination)
	  {
		 case OUTPUTD_DESTINATION:
			printf("%ld\n", next_token.data);
			fflush(stdout);
			break;

		 case OUTPUTS_DESTINATION:
			// vuln: could use this to leak out the next part of the token,
			// might be useful for exploitation.
			printf("%s", (char*)&next_token.data);
			fflush(stdout);
			break;

		 case REGISTER_INPUT_HANDLER_DESTINATION:
			#ifdef DEBUG
			fprintf(stderr, "TODO: unimplemented register input handler");
			print_token(next_token);
			#endif
			break;

		 case DEREGISTER_INPUT_HANDLER_DESTINATION:
			#ifdef DEBUG
			fprintf(stderr, "TODO: unimplemented deregister input handler");
			print_token(next_token);
			#endif
			break;

		 case DEV_NULL_DESTINATION:
			// Just consume the token, it's not meant for anywhere (/dev/null)
			#ifdef DEBUG
			fprintf(stderr, "Ignoring this token:\n");
			print_token(next_token);
			#endif
			break;

		 default:
			queue_add(matching_unit_input_queue, &next_token, sizeof(token_type));
	  }
   }
}
