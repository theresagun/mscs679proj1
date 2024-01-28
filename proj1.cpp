/**
 * TODOs
 *  - run with valgrind if we use malloc
 *  - track memory usage
 *  - fix when it counts whitespace as a word (happens when using one thread on simpleTest)
 *  - when running simpleTest with 3 threads it counts "s" as its own word?
 *  - output to file
*/

#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <chrono>

using namespace std;


/* need a mutex var for the threads to wait on */
mutex mtx;

class MapNode {
    public:
        string word;
        int numOccurences; // TODO - int or long? 
        MapNode* next;
        MapNode* prev;
        MapNode(string w, MapNode* p) {
            word = w;
            numOccurences = 1;
            next = nullptr;
            prev = p;
        }
};

class MapLinkedList {
    public: 
       MapNode* head;
       MapNode* tail;
       MapLinkedList() {
        head = nullptr;
        tail = nullptr;
       }

       void incrementCount(string word) {
        // cout << "in incrementCount for: " << word << endl;
        if (head == nullptr) {
            // cout << "creating head node" << endl;
            // if this is the first element
            MapNode* n = new MapNode(word, nullptr);
            head = n;
            tail = n;
            return;
        } else {
            // cout << "not head" << endl;
            if (head->word == word) {
                // cout << "word was head word" << endl;
                // the first word is the word we are looking for
                head->numOccurences++;
                return;
            }
            // not the first element
            MapNode* temp = head;
            while(temp->next != nullptr) {
                // cout << "traversing... " << temp->word  <<" "<< temp->numOccurences << endl;
                temp = temp->next;
                if (temp->word == word) {
                    // cout << "found the word" << endl;
                    // found the word, increment 
                    temp->numOccurences++;
                    // ensure node is in correct sorted place
                    MapNode* sorterTemp = temp->prev; // what we are using to traverse
                    MapNode* swapper = temp;
                    if (sorterTemp->numOccurences <= swapper->numOccurences) {
                        temp->prev->next = temp->next;
                        temp->next->prev = temp->prev;
                    }
                    while(sorterTemp->prev != nullptr && sorterTemp->numOccurences <= swapper->numOccurences) { // 
                        // cout << "   traversing... " << sorterTemp->word << " " << sorterTemp->numOccurences << " vs " << swapper->numOccurences << endl;
                        // traverse:
                        sorterTemp = sorterTemp->prev;
                    }
                    if (sorterTemp == head && sorterTemp->numOccurences <= swapper->numOccurences) {
                        cout << "    replacing head" << endl;
                        swapper->prev = nullptr;
                        swapper->next = head;
                        head->prev = swapper;
                        head = swapper;
                    } else if (sorterTemp->numOccurences > swapper->numOccurences) { // does this need to be else if or else???
                        cout << "    stitching in" << endl;
                        // stitch it into correct spot
                        sorterTemp->next->prev = swapper;
                        swapper->next = sorterTemp->next->prev;
                        swapper->prev = sorterTemp;
                        sorterTemp->next = swapper;
                    } else {
                        cout<< "\n\nELSE CASE!!!!\n\n"<<endl; // maybe if it's second most occurences ?
                    }
                    return;
                }
            }
            // cout << "did not find the word" << endl;
            // the word is not yet in the list
            MapNode* n = new MapNode(word, temp);
            temp->next = n;
            tail = n;
        }
       }

};

bool isLetter(char c) {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return true;
    return false;
}

/* 
* convert to lowercase by adding the difference between 'a' and 'A' which is 32 
*/
char toLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}


/* new threads start here */
void countWords (const char* contents, int* sizePtr, int*tId, void* c) {
    MapLinkedList* counter = (MapLinkedList*) c;
    int size = *sizePtr;
    int frontIndex = 0;
    int backIndex = 1;
    char currChar;
    // while we are within the bounds of this chunk of contents
    while (frontIndex < size) {
        // get to the start of the word
        // cout << "original front index is " << frontIndex << " giving us " << *(contents+frontIndex) << endl;
        while (!isLetter(*(contents+frontIndex))) { // TODO account for apostrophes?
            // cout << "adjusting front index" << endl;
            frontIndex++;
            backIndex = frontIndex + 1;
            if (backIndex>size) break;
        }
        char beginningChar = *(contents+frontIndex);
        currChar = *(contents+backIndex);
        // get to the end of the word (accept letters, apostrophes (39), and dashes (45))
        while (isLetter(currChar) || currChar == 39 || currChar == 45) { 
            // cout << "adjusting back index" << endl;
            backIndex++;
            currChar = *(contents+backIndex);
        }

        // cout << "fInd: " << frontIndex << " bInd: " << backIndex << endl;
        string word = "";
        for (int in = frontIndex; in < backIndex; in++) {
            word += toLower(*(contents+in));
        }

        { // scope for mutex to work in
            // cout << "thread " << *tId << " wants to acquire mutex" << endl;
            lock_guard<mutex> lock{mtx};
            // cout << "thread " << *tId << " acquired mutex" << endl;
            counter->incrementCount(word);
            // cout << "thread " << *tId << " is done with mutex" << endl;
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

    // start timer (TODO do we want this here or after reading file input?)
    auto start = chrono::high_resolution_clock::now();

    /* get the text from the text file */
    string fileContents = "";

    string line;
    ifstream fin;
    ofstream fout;

    fin.open(filename);

    // Some error handling to make sure the file stream successfully opened the file
    if (!fin) {
        cerr << "Error: Unable to open file '" << filename << "'" << endl;
        return -1; // non-zero value indicates error
    }
    
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

    MapLinkedList* counter = new MapLinkedList();

    // iterate from endPosition forward to find the next non letter
    // set this to the endPosition
    // for the next interval set the startPosition to the old endPosition (+1?)
    thread threads[numThreads];
    int threadIds[numThreads];
    char arrayOfFileChunks[numThreads][intervalLength+15]; //+15 is random for the adjustment of endPosition below
    for (int i = 0; i < numThreads; i++) {
        while (isLetter(fileContents[endPosition]) && fileContents.length()>endPosition) endPosition++;
        cout << "thread " << i << " starting at " << startPosition << " ending at " << endPosition << endl;
        // get this content chunk
        string fileChunk = fileContents.substr(startPosition, (endPosition - startPosition));
        // save the chunk into array so the data stays in memory long enough
        fileChunk.copy(&arrayOfFileChunks[i][0], fileChunk.size());      
        // start up a new thread and save it in the array to keep track of it
        threadIds[i] = i;
        threads[i] = thread(countWords, &arrayOfFileChunks[i][0], &intervalLength, &threadIds[i], (void*)counter);
        // update the start and end positions for the next segment
        startPosition = endPosition;
        endPosition = startPosition + intervalLength;
    }

    /* wait for all threads to finish */
    for (int i = 0; i < numThreads; i++) threads[i].join();

    // end timer
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);


    /* TODO write output to file
        - should it write at the end? or have access to the file be mutex so we don't need a map
        - feels faster to write at the end so we have less file accessing - fine if we can use map
    */

    // just for testing now
    cout << "\nTotal time is " << duration.count() << " milliseconds using " << numThreads << " threads" << endl;
    MapNode* temp = counter->head;
    cout << temp->word << " " << temp->numOccurences << endl;
    while (temp->next != nullptr) {
        temp = temp->next;
        cout << temp->word << " " << temp->numOccurences << endl;
    }

    return 0;
}
