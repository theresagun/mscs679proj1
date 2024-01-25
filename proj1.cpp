/**
 * TODOs
 *  - run with valgrind if we use malloc
 *  - track memory usage
 *  - track time
 *  - get permission to use map library
*/

#include <thread>
#include <mutex>
#include <semaphore>
#include <atomic>
#include <iostream>
#include <fstream>
#include <cstdlib>
// can't include anything else?
// TODO go through and remove map if true
#include <map>
#include <vector>

using namespace std;

/* global var to hold a counter for each word... map for now */
map<string, int> COUNTER;

/* need a mutex var for the threads to wait on */
mutex mtx;

counting_semaphore sem(1);

bool isLetter(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return false;
}

// TODO rewrite this once it's working
char toLower(char c) {
    if (c <= 'Z' && c >= 'A')
        return c - ('Z' - 'z');
    return c;
}

/* new threads start here */
void countWords (char* contents, int* sizePtr) {
    int size = *sizePtr;
    int frontIndex = 0;
    int backIndex = 1;
    char currChar;
    // while we are within the bounds of this chunk of contents
    while (frontIndex < size) {
        // get to the start of the word
        while (!isLetter(*(contents+frontIndex))) { // TODO account for apostrophes?
            frontIndex++;
            backIndex = frontIndex + 1;
            if (backIndex>size) break;
        }
        char beginningChar = *(contents+frontIndex);
        currChar = *(contents+backIndex);
        // get to the end of the word (accept letters, apostrophes (39), and dashes (45))
        while (isLetter(currChar) || currChar == 39 || currChar == 45) { 
            backIndex++;
            currChar = *(contents+backIndex);
        }

        string word = "";
        for (int in = frontIndex; in < backIndex; in++) {
            word += toLower(*(contents+in));
        }

        { // scope for semaphore and mutex to work in
            sem.acquire();
            mtx.lock(); // TODO lock_guard?
            if (COUNTER.find(word) == COUNTER.end()) {
                // this is the first occurrence 
                COUNTER[word] = 1;
            } else {
                // this is not the first occurrence - increment count
                COUNTER[word]++; 
            }
            mtx.unlock();
            sem.release();
        }

        // update the index
        frontIndex = backIndex;
        backIndex++;
    }
}

int main(int argc, char** argv) {
    /* take input from command line - input file name and number of threads */
    /* ./proj1 filename.txt numThreads */
    if (argc != 3) {
        printf("Expected 2 arguments. Example usage: ./proj1 filename.txt 4");
        return -1;
    }
    string filename = argv[1];
    int numThreads = atoi(argv[2]);

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

    /* divide it up into the number of parts (based on num of threads)
        divide and search for the next non letter character (end of a word).
        update the next threads starting index to be after that */

    int intervalLength = fileContents.length()/numThreads;
    int startPosition = 0;
    int endPosition = intervalLength - 1; // TODO double check where i need to adjust by 1 in all this math

    // iterate from endPosition forward to find the next non letter
    // set this to the endPosition
    // for the next interval set the startPosition to the old endPosition (+1?)
    vector<thread> threads;
    for (int i = 0; i < numThreads; i++) {
        cout << "thread num " << i << endl;
        while (isLetter(fileContents[endPosition]) && fileContents.length()>endPosition) endPosition++;
        cout << "starting at " << startPosition << " ending at " << endPosition << endl;
        // get this content chunk
        string fileChunk = fileContents.substr(startPosition, (endPosition - startPosition));
        // start up a new thread
        thread t(countWords, (char*)fileChunk.c_str(), &intervalLength);
        // add to list so we don't lose it
        // threads.push_back(t); // TODO error
        t.join(); // for the simple case of 1 thread until fix above error
        // update the start and end positions for the next segment
        startPosition = endPosition;
        endPosition = startPosition + intervalLength;
    }

    /* wait for all threads to finish */
    for (int i = 0; i < threads.size(); i++) threads[i].join();

    /* TODO write output to file
        - should it write at the end? or have access to the file be mutex so we don't need a map
        - feels faster to write at the end so we have less file accessing - fine if we can use map
    */

    // just for testing now
    for (const auto& pair : COUNTER) {
        cout << "Key: " << pair.first << " Value: " << pair.second << endl;
    }

    return 0;
}
