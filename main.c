 #include <stdio.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

jack_port_t *input_port;
jack_port_t *output_port;

int process (jack_nframes_t nframes, void *arg)
{
	int i = 0;
	jack_default_audio_sample_t *out = 
                (jack_default_audio_sample_t *) 
                jack_port_get_buffer (output_port, nframes);
	jack_default_audio_sample_t *in = 
                (jack_default_audio_sample_t *) 
                jack_port_get_buffer (input_port, nframes);
	printf("%d\n", nframes);
	memcpy (out, in, sizeof (jack_default_audio_sample_t) * nframes);
	
	return 0;      
}

int srate (jack_nframes_t nframes, void *arg)
{
	printf ("the sample rate is now %lu/sec\n", nframes);
	return 0;
}

void error (const char *desc)
{
	fprintf (stderr, "JACK error: %s\n", desc);
}

void jack_shutdown (void *arg)
{
	exit (1);
}

int main (int argc, char *argv[])
{
        /*KIRBY: Create a JACK client.  
         This is our connection to the JACK daemon.*/

	jack_client_t *client;

	/*KIRBY: A pointer for an array of ports.  Remember, we 
          already saw this being 
	  used to hold the array of available ports
	  See the previous chapter*/

	const char **ports;


	/*KIRBY: This doesn't really need an explanation*/

	if (argc < 2) {
		fprintf (stderr, "usage: jack_simple_client \n");
		return 1;
	}

	/* tell the JACK server to call error() whenever it
	   experiences an error.  Notice that this callback is
	   global to this process, not specific to each client.
	
	   This is set here so that it can catch errors in the
	   connection process
	*/
	/*KIRBY: Nuff said.*/
	jack_set_error_function (error);

	/* try to become a client of the JACK server */
	/*KIRBY:  This is where our pointer "client" 
          gets something to point to.  
	  You will notice that the functions called later take a client as 
	  a parameter - this is what we pass.*/
	if ((client = jack_client_new (argv[1])) == 0) {
		fprintf (stderr, "jack server not running?\n");
		return 1;
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, 0);

	/* tell the JACK server to call `srate()' whenever
	   the sample rate of the system changes.
	*/

	jack_set_sample_rate_callback (client, srate, 0);

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* display the current sample rate. once the client is activated 
	   (see below), you should rely on your own sample rate
	   callback (see above) for this value.
	*/
	printf ("engine sample rate: %lu\n", jack_get_sample_rate (client));

	/* create two ports */

	input_port = jack_port_register (client, "input", 
                     JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
	output_port = jack_port_register (client, "output", 
                     JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

	/* tell the JACK server that we are ready to roll */
	/*KIRBY: So, once we are in a position to start 
          doing whatever it is we do, this is how we announce that.*/
	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		return 1;
	}

	/* connect the ports. Note: you can't do this before
	   the client is activated, because we can't allow
	   connections to be made to clients that aren't
	   running.
	*/
	/*KIRBY: We already discussed this.  Go back a chapter 
                 if you missed it.*/
	
	if ((ports = jack_get_ports (client, NULL, NULL, 
                               JackPortIsPhysical|JackPortIsOutput)) == NULL) {
		fprintf(stderr, "Cannot find any physical capture ports\n");
		exit(1);
	}

	if (jack_connect (client, ports[0], jack_port_name (input_port))) {
		fprintf (stderr, "cannot connect input ports\n");
	}

	free (ports);
	
	if ((ports = jack_get_ports (client, NULL, NULL, 
                               JackPortIsPhysical|JackPortIsInput)) == NULL) {
		fprintf(stderr, "Cannot find any physical playback ports\n");
		exit(1);
	}
	
	/*KIRBY: This is our modified bit.  Groovy, eh?*/
	
	int i=0;
	while(ports[i]!=NULL){
	  if (jack_connect (client, jack_port_name (output_port), ports[i])) {
	    fprintf (stderr, "cannot connect output ports\n");
	  }
	  i++;
	}

	free (ports);

	/* Since this is just a toy, run for a few seconds, then finish */
	/*KIRBY: We changed that, too.  Now we run until we get killed.*/
	for(;;)
	  sleep (1);

	/*KIRBY: Close the client*/
	jack_client_close (client);
	exit (0);
}