// Gregor McWilliam

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>     // sleep()
#include <sndfile.h>	// libsndfile
#include <portaudio.h>	// PortAudio
#include <ncurses.h> 	// GUI
#include <stdatomic.h>
#include "paUtils.h"

const int MAX_PATH_LEN = 256;
const int MAX_FILES = 8;
const int MAX_CHAN = 2;
const int FRAMES_PER_BUFFER = 1024;

// Data structure to pass to callback
struct Buf {
	atomic_int selection;	// So selection is thread-safe
	unsigned int channels;
	unsigned int samplerate;
	float *x[MAX_FILES];
	unsigned long frames[MAX_FILES];
	unsigned long nextFrame[MAX_FILES];
	unsigned long frameNum[MAX_FILES];
	char trackTitle[MAX_FILES][MAX_PATH_LEN];
	bool playback;
	bool rew;
	bool fwd;
	bool stop;
	bool jumpBack;
	bool jumpFwd;
	bool loop;
};

void printStartScreen(void) {
	printw("        .------------------------.  \n");
	printw("        |\\////////         G.M.  | \n");
	printw("        | \\/  __  ______  __     | \n");
	printw("        |    /  \\|\\.....|/  \\    | \n");
	printw("        |    \\__/|/_____|\\__/    | \n");
	printw("        |     = TRANSPORTR =     | \n");
	printw("        |    ________________    | \n");
	printw("        |___/_._o________o_._\\___| \n");
	printw("\n");
	printw("          PLAY/PAUSE:  SPACEBAR  	\n");
	printw("        ========================== \n");
	printw("  STOP:        S    	  SKIPBACK:    <	\n");
	printw("  REW:         A          SKIPFWD:     >	\n");
	printw("  FFWD:        D          					\n");
	printw("  QUIT:        Q    	  LOOP:	       L    \n");
	curs_set(0);
}

// PortAudio callback function protoype
static int paCallback( const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData );

int main(int argc, char *argv[])
{
  	char ifilename[MAX_FILES][MAX_PATH_LEN], ch;
  	int i, selection;
  	float count;
  	unsigned int numInputFiles;
  	FILE *fp;

  	// Declare instance of Buf struct
  	Buf buf, *p = &buf;

  	// Declare libsndfile structures
	SNDFILE *sndfile; 
	SF_INFO sfinfo;

  	// Declare pointer to PortAudio stream
	PaStream *stream;

  	// Zero libsndfile structures
	std::memset(&sfinfo, 0, sizeof(sfinfo));

	// initialize selection: -1 indicates don't play any file
	p->selection = -1;
	p->playback = false;
	p->stop = false;
	p->rew = false;
	p->fwd = false;

  	// ===============================================================
  	// ============ WAV file management and error checks =============
  	// ===============================================================

	// Direct file pointer to command line argument
	fp = fopen("audioPaths.txt", "r");

	// If fp points to null value, print error statement and return -1
	if (!fp) {
		std::cerr << "Cannot open " << argv[1] << std::endl;
		return -1;
	}

	/* Read WAV filenames from the list in ifile_list.txt */
	/* Print information about each WAV file */

	// Iterate through all files
	for (i=0; i<MAX_FILES; i++) {

		// If end of ifilename array is reached, break out of loop
		if (fscanf(fp, "%s", ifilename[i]) == EOF)
			break;

		// Open ith element of ifilename with sndfile function
		sndfile = sf_open(ifilename[i], SFM_READ, &sfinfo);

		// If no valid information, return error statement and return -1
		if (!sndfile) {
			std::cerr << "Cannot open " << ifilename[i] << std::endl;
			return -1;
		}

		// char trackTitle[MAX_PATH_LEN];
		strncpy(p->trackTitle[i], ifilename[i]+8, sizeof(ifilename[i]) - sizeof(".wav"));
		// p->trackTitle[i] = trackTitle;

		// Otherwise, print header information for user
		std::cout << i << " " << "Frames: " << sfinfo.frames << ", Channels: " << sfinfo.channels
				  << " Samplerate: " << sfinfo.samplerate << ", " << p->trackTitle[i] << std::endl;

		// Set number of input files to 0
		numInputFiles = 0;
		
		// Retrieve frame information and assign to Buf structure
		p->frames[i] = sfinfo.frames;

		// Set next frame to 0
		p->nextFrame[i] = 0;

		// Set frame num to 0
		p->frameNum[i] = 0;

		// On the first iteration, retrieve channel and sample rate information
		if (i == 0) {
			p->channels = sfinfo.channels;
			p->samplerate = sfinfo.samplerate;
		}

		// If number of channels beyond pre-determined max, print error statement
		// and return -1
		if (sfinfo.channels > MAX_CHAN) {
			std::cerr << "Error: " << ifilename[i] << " contains too many channels" << std::endl;
			return -1;
		}

		// If any files have a channel count inconsistent with the first file,
		// print error statement and return -1
		if (i > 0 && sfinfo.channels != p->channels) {
			std::cerr << "Error: Inconsistent channel counts" << std::endl;
			return -1;
		}

		// If any files have a sample rate inconsistent with the first file,
		// print error statement and return -1
		if (i > 0 && sfinfo.samplerate != p->samplerate) {
			std::cerr << "Error: Inconsistent sample rates" << std::endl;
			return -1;
		}

		// Allocate heap storage and read audio data into buffer
		p->x[i] = new float[p->frames[i] * p->channels];

		// If no valid audio data, print error statement and return -1
		if (!p->x[i]) {
			std::cerr << "Error: Unable to read audio file" << std::endl;
			return -1;
		}

		// Extract frame information and assign to variable count
		count = sf_readf_float(sndfile, p->x[i], p->frames[i]);

		// If count is inconsistent with header-derived frame number, print
		// error statement and return -1
		if (count != p->frames[i]) {
			std::cerr << "Error: Inconsistent audio frame count" << std::endl;
			return -1;
		}

  		// Close WAV file
  		sf_close(sndfile);
   	}

   	// Assign number of files iterated through to variable
   	numInputFiles = i;

   	// Close input filelist
   	fclose(fp);

	// 2-second pause so user can read console printout
	// usleep(2 * 1000000);

   	// Set track 0 as default selection
	p->selection = 0;

	// Start up Port Audio
	stream = startupPa(1, p->channels, p->samplerate, 
		FRAMES_PER_BUFFER, paCallback, &buf);

	// ===============================================================
  	// ============ ncurses ==========================================
  	// ===============================================================

  	// Initialize ncurses to permit interactive character input 
  	initscr(); // Start curses mode
  	cbreak();  // Line buffering disabled
  	noecho();

	printw("SELECT TRACK BY NUMBER, THEN PRESS PLAY:\n");
	for (i=0; i<numInputFiles; i++) {
	  	printw("%2d: %s\n", i, p->trackTitle[i]);
	}
	printw("\n\n");
  	refresh();

  	int curMins, curSecs, durMins, durSecs = 0;

  	// Init ch as null character
	ch = '\0';

	while (ch != 'q') {

		// Calculate current minutes:seconds and total minutes:seconds
		curMins = ((FRAMES_PER_BUFFER * p->frameNum[selection]) / p->samplerate) / 60;
		curSecs = ((FRAMES_PER_BUFFER * p->frameNum[selection]) / p->samplerate) - (60*curMins);
		durMins = (p->frames[selection] / p->samplerate) / 60;
		durSecs = ((p->frames[selection] / p->samplerate) - (60*durMins));

		printw("\n");

		refresh();
		printStartScreen();
		move(22, 15);

		// Print current position in song (mins:secs/mins:secs)
		printw("%02d:%02d/%02d:%02d", curMins, curSecs, durMins, durSecs);
		move(23, 17);

		// Await user input
		nodelay(stdscr, true);
		ch = tolower(getch()); 

		// If STOP true as end of file reached, print feedback to screen
		// and handle transport state logic
		if (p->stop) {
			printw("STOPPED\n");
			p->playback = false;
			p->stop = false;
		}

		// If spacebar pressed, pause playback at current frame
		if (ch == ' ') {

			p->stop = false;

			// Set REW and FWD states to false
			if (p->rew || p-> fwd) {
				p->rew = false;
				p->fwd = false;
			}

			// Otherwise, switch playback state
			else {
				p->playback = !p->playback;
			}

			// If playing, print user feedback and set STOP to false
			if (p->playback || p->stop) {
				printw("PLAYING\n");
				p->stop = false;
			}

			// Otherwise paused, and print user feedback
			else {
				printw("PAUSED\n");
			}

		}

		// If 's' pressed, set playback to false, reset next frame to 0, and print feedback
		else if (ch == 's') {

			p->playback = false;
			p->stop = true;
			p->nextFrame[selection] = 0;
			p->frameNum[selection] = 0;
			printw("STOPPED\n");
		}

		// If ',' pressed, set jumpBack to true and jumpFwd to false
		else if (ch == ',' && p->playback) {

			p->jumpBack = true;
			p->jumpFwd = false;

		}

		// If '.' pressed, set jumpFwd to true and jumpBack to false
		else if (ch == '.' && p->playback) {

			p->jumpFwd = true;
			p->jumpBack = false;

		}

		// If 'a' pressed, set FWD to false, switch REW state, and print feedback
		// If REW already true, normal playback resumes
		else if (ch == 'a' && p->playback) {

			p->fwd = false;
			p->rew = !p->rew;

			if (p->rew) {
				printw("REW\n");
			}
			else {
				printw("PLAYING\n");
			}
		}

		// If 'd' is pressed, set REW to false, switch FWD state, and print feedback
		// If REW already true, normal playback resumes
		else if (ch == 'd' && p->playback) {

			p->rew = false;
			p->fwd = !p->fwd;

			if (p->fwd) {
				printw("FWD\n");
			}
			else {
				printw("PLAYING\n");
			}
		}

		else if (ch == 'l') {

			p->loop = !p->loop;

			if (p->loop) {
				printw("LOOPING");
			}
			else {
				printw("NOT LOOPING");
			}
		}

		// If numeral input, assign to selection variable and switch track via struct
		else if (ch >= '0' && ch < '0'+numInputFiles) {
			selection = ch-'0';

			// == Write info to be read in callback ==
			// This one-way communication of a single item eliminated possibility
			// of race condition due to asynch threads

			p->selection = selection;
		}

		mvprintw(numInputFiles+2, 0, "CPU Load: %lf\n", Pa_GetStreamCpuLoad (stream) );
		mvprintw(numInputFiles+3, 0, "Selected Track: %d - %s", selection, p->trackTitle[selection]);
		refresh();

  	}

  	// Shut down Ncurses
  	endwin();

	// Shut down PortAudio
	shutdownPa(stream);
	 
	return 0;
}

// ===============================================================
// ============ PortAudio callback ===============================
// ===============================================================

// This routine will be called by the PortAudio engine when audio is needed.
// It will be called in the real-time thread, so must contain only non-blocking code

static int paCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData)
{

	Buf *p = static_cast<Buf*>(userData); // Cast data passed through stream to our structure. 
	float *output = static_cast<float*>(outputBuffer); // Input not used in this code

	atomic_int selection;

	// Local variable selection assigned
	selection = p->selection;

	// If selection is -1, fill output buffer with zeros
	if (selection == -1 || !p->playback) {

		for (int i = 0; i < framesPerBuffer * p->channels; i++) {
			output[i] = 0.0;
		}
	}

	// Otherwise, iterate through current buffer
	else {

		// Declare / initialize variables
		int next_sample, k = 0;

		// If next frame greater than total number of frames, set next frame to 0
		if (p->nextFrame[selection] + framesPerBuffer >= p->frames[selection]) {

			if (!p->loop) {
				p->stop = true;
				p->playback = false;
				p->rew = false;
				p->fwd = false;
			}

			p->nextFrame[selection] = 0;
			p->frameNum[selection] = 0;

		}

		// Otherwise, set next sample as next frame * number of channels
		next_sample = p->nextFrame[selection] * p->channels;

		// If REW not selected, iterate through current buffer as normal
		if (!p->rew) {
			for (int i = 0; i < framesPerBuffer; i++) {
				for (int j = 0; j < p->channels; j++) {
					output[k] = p->x[selection][next_sample + k];
					k++;
				}
			}
		}

		// Otherwise, REW selected, so iterate through current buffer in reverse
		else {
			for (int i = framesPerBuffer-1; i > -1; i--) {
				for (int j = p->channels-1; j > -1; j--) {
					output[k] = p->x[selection][next_sample + k];
					k++;
				}
			}
		}

		// If JUMPBACK selected, next frame is closest int divisor of
		// fs/framesPerBuffer frames back, to approximate a one-second jump
		if (p->jumpBack) {
			p->nextFrame[selection] -= (p->samplerate / framesPerBuffer) * framesPerBuffer;
			p->jumpBack = false;
			p->frameNum[selection] -= (p->samplerate / framesPerBuffer);
		}

		// If JUMPFWD selected, next frame is closest int divisor of 
		// fs/framesPerBuffer frames forward, to apprxoimate a one-second jump
		else if (p->jumpFwd) {
			p->nextFrame[selection] += (p->samplerate / framesPerBuffer) * framesPerBuffer;
			p->jumpFwd = false;
			p->frameNum[selection] += (p->samplerate / framesPerBuffer);
		}
		
		// If REW selected, next frame is TWO frame BEFORE current frame
		else if (p->rew) {
			p->nextFrame[selection] -= 2*framesPerBuffer;
			p->frameNum[selection] -= 2;
		}

		// If FWD selected, next frame is TWO frames AFTER current frame
		else if (p->fwd) {
			p->nextFrame[selection] += 2*framesPerBuffer;
			p->frameNum[selection] += 2;
		}

		// Otherwise, playback speed is normal, so next frame ONE frame AFTER current frame
		else {
			p->nextFrame[selection] += framesPerBuffer;
			p->frameNum[selection]++;
		}

	}

	return 0;
}