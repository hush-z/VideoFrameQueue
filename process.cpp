#include <atomic>
#include <iostream>
#include "helper.h"
#include <thread>
ThreadSafeFIFOBuffer<MatData> theBuffer(200);
atomic<bool> grabbing, processing;
void GrabberThread_Test(char*, int);
void ProcessorThread_Test();
void do_work(MatData &data);

void GrabberThread_Test(char* videoPath, int interv)
{
	VideoCapture capIr;
	capIr.open(videoPath);
	MatData grabData;
	int frameCount = 0;
	int interval = interv;
	int retry = 50;
	while(1)
	{
		capIr >> grabData.ir;
		if (grabData.ir.empty()) // nothing to grab
			break; // Done grabbing
		frameCount++;
		grabData.vars.timestamp = getTickCount();
		grabData.vars.frameNum = frameCount;
		if (theBuffer.Push(grabData))
		{
			// Every interval milliseconds push a frame into the buffer
			this_thread::sleep_for(chrono::milliseconds(interval));
		}
		else
		{
			cout << "Buffer is full" << endl;
			while(!theBuffer.Push(grabData)){
				this_thread::sleep_for(chrono::milliseconds(retry));
			}
		}

	}
	grabbing.store(false); // Done grabbing
}

void ProcessorThread_Test()
{
	MatData procData;
	Mat ir, depth;

	while(processing.load()==true){
		int flag = theBuffer.PopAndCount(procData);
		if(flag==-1){
			if(grabbing.load()==true){
				continue;
			}else{
				processing.store(false);
			}
		}else{
			if(flag==0){
				cout << "Frame "<< procData.vars.frameNum << " Meet Real-time" << endl;
			}else{
				cout << "Frame "<< procData.vars.frameNum << " Miss Deadline" << endl;
			}
			do_work(procData);	
		}
	}
	cout << "Done processing" << endl;
}

void do_work(MatData &data){
	int sleep = 50;
	this_thread::sleep_for(chrono::milliseconds(sleep));
}

int main(int argc, char* argv[]) {
	if(argc!=3){
		cout << "Usage: process <video_file> <interval_in_ms> " << endl;
		return -1;
	}
	char* videoPath = argv[1];
	int interval = atoi(argv[2]);


	grabbing.store(true);               // set the grabbing control variable
	processing.store(true);             // ensure the processing will start
	
	thread grab(GrabberThread_Test, videoPath, interval);    // start the grabbing task
	thread proc(ProcessorThread_Test);  // start the the processing task
	
	grab.join();              // wait for the grab thread
	proc.join();              // wait for the process thread
	return 0;
}
