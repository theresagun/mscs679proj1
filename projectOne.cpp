#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>

using namespace std;

const int INITIAL_CAPACITY = 10; // Initial size of the dynamic array

struct WordCount {
    string word;
    int count;
};

void quickSort(WordCount* arr, int low, int high);
int partition(WordCount* arr, int low, int high);

WordCount* wordCounts = new WordCount[INITIAL_CAPACITY];
int wordCountSize = 0;
int wordCountCapacity = INITIAL_CAPACITY;
mutex wordCountMutex;

// Function to resize the dynamic array
void resizeWordCounts() {
    int newCapacity = wordCountCapacity * 2;
    WordCount* newWordCounts = new WordCount[newCapacity];

    for (int i = 0; i < wordCountSize; ++i) {
        newWordCounts[i] = wordCounts[i];
    }

    delete[] wordCounts;
    wordCounts = newWordCounts;
    wordCountCapacity = newCapacity;
}

// Function to convert character to lowercase
char toLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

// Function to process each text chunk
void processTextChunk(const string& textChunk) {
    string word;
    for (char c : textChunk) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == 39) {
            // buildup a word with letters, hyphens, and apostrophes
            word += toLower(c);
        }
        else if (!word.empty()) {
            // if there is a space, number or special character then that's the end of the word
            lock_guard<mutex> guard(wordCountMutex);
            bool found = false;

            for (int i = 0; i < wordCountSize; ++i) {
                if (wordCounts[i].word == word) {
                    wordCounts[i].count++;
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (wordCountSize == wordCountCapacity) {
                    resizeWordCounts();
                }
                wordCounts[wordCountSize++] = { word, 1 };
            }

            word.clear();
        }
    }

    if (!word.empty()) {
        // handle the last word in the chunk if it wasn't already handled
        lock_guard<mutex> guard(wordCountMutex);
        bool found = false;

        for (int i = 0; i < wordCountSize; ++i) {
            if (wordCounts[i].word == word) {
                wordCounts[i].count++;
                found = true;
                break;
            }
        }

        if (!found) {
            if (wordCountSize == wordCountCapacity) {
                resizeWordCounts();
            }
            wordCounts[wordCountSize++] = { word, 1 };
        }
    }
}

/* Serial merge */
void merge(WordCount arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    WordCount* L = new WordCount[n1];
    WordCount* R = new WordCount[n2];

    //WordCount L[n1], R[n2];

    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    int i = 0;
    int j = 0;
    int k = l;

    while (i < n1 && j < n2) {
        if (L[i].count >= R[j].count) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    delete[] L;
    delete[] R;

}

/* Semi parallel merge sort */
void mergeSortParallel(WordCount arr[], int l, int r, int depth) {
    if (l < r) {
        if (depth <= 0) {
            // If depth limit is reached, switch to sequential merge sort
            mergeSortParallel(arr, l, (l + r) / 2, 0);
            mergeSortParallel(arr, (l + r) / 2 + 1, r, 0);
        }
        else {
            thread t1(mergeSortParallel, arr, l, (l + r) / 2, depth - 1);
            thread t2(mergeSortParallel, arr, (l + r) / 2 + 1, r, depth - 1);
            t1.join();
            t2.join();
        }

        merge(arr, l, (l + r) / 2, r);
    }
}

int main(int argc, char** argv) {
    cout << "Program Starting" << endl;
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <inputFile> <numberOfThreads>\n";
        return 1;
    }

    string inputFile = argv[1];
    ifstream file(inputFile);
    if (!file) {
        cerr << "Error opening file: " << inputFile << "\n";
        return 1;
    }

    string fileContents((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    int numThreads = atoi(argv[2]);
    ofstream outputFile("output.txt");

    auto start = chrono::high_resolution_clock::now();
    thread* threads = new thread[numThreads];
    //thread threads[numThreads];
    size_t chunkSize = fileContents.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i + 1) * chunkSize;

        if (i == numThreads - 1) {
            end = fileContents.size();
        }

        // Ensure we don't split words between chunks
        if (end < fileContents.size()) {
            while (end < fileContents.size() && (isalpha(fileContents[end]) || fileContents[end] == '-' || fileContents[end] == 39)) {
                end++;
            }
        }

        string chunk = fileContents.substr(start, end - start);
        threads[i] = thread(processTextChunk, chunk);
    }

    // Wait for all threads to finish
    for (size_t i = 0; i < numThreads; ++i) {
        threads[i].join();
    }

    // Sort the word counts
    mergeSortParallel(wordCounts, 0, wordCountSize - 1, 2);

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> runtime = end - start;

    cout << "Runtime: " << runtime.count() << endl;
    // Write the sorted word counts to the outputFile
    for (int i = 0; i < wordCountSize; ++i) {
        outputFile << wordCounts[i].word << " " << wordCounts[i].count << "\n";
    }
    outputFile.close();
    // Clean up the dynamic array
    delete[] wordCounts;
    return 0;
}




