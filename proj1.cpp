#include <thread>
#include <mutex>
#include <semaphore>
#include <atomic>
#include <iostream>
#include <fstream>
// can't include anything else?
// TODO go through and remove string and map if true
#include <string>
#include <map>

using namespace std;

/* global var to hold a counter for each word... map for now */
map<string, integer> COUNTER;

/* need a mutex var for the threads to wait on */
mutex mtx;

/* need a method for the threads to start at */
void count(string contents) {
    // TODO
}

bool isLetter(char c) {
    if ((c >= 65 && c <=90) || (c >= 97 || c <= 122)) return true;
    return false;
}

int main(int argc, char **argv) {
    /* take input from command line - input file name and number of threads */
    /* ./proj1 filename.txt numThreads */
    if (argc != 3) {
        printf("Expected 2 arguments. Example usage: ./proj1 filename.txt 4");
        return -1;
    }
    string filename = argv[1];
    int numThreads = argv[2];

    /* get the text from the text file */
    string fileContents = "";

    string line;
    ifstream fin;
    ofstream fout;

    fin.open(filename);
    while (getline(fin, line)) {
        fileContents.append(line);
    }
    fin.close();

    // we don't care about case
    fileContents = fileContents.tolower();

    /* divide it up into the number of parts (based on num of threads)
        divide and search for the next non letter character (end of a word).
        update the next threads starting index to be after that */

    int intervalLength = fileContents.length()/numThreads;
    int startPosition = 0;
    int endPosition = intervalLength - 1; // TODO double check where i need to adjust by 1 in all this math

    // iterate from endPosition forward to find the next non letter
    // set this to the endPosition
    // for the next interval set the startPosition to the old endPosition (+1?)
    list<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        while (isLetter(fileContents[endPosition])) endPosition++;
        // get this content chunk
        string fileChunk = fileContents.substr(startPosition, (endPosition - startPosition));
        // start up a new thread
        thread t(count, fileChunk);
        // add to list so we don't lose it
        threads.push_back(t);
        // update the start and end positions for the next segment
        startPosition = endPosition;
        endPosition = startPosition + intervalLength;
    }

    /* wait for all threads to finish */
    for (int i = 0; i < threads.length(); i++) threads[i].join();

    /* TODO write output to file
        - should it write at the end? or have access to the file be mutex so we don't need a map
        - feels faster to write at the end so we have less file accessing - fine if we can use map
    */

    return 0;
}