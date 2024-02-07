#include <iostream>
#include <fstream>
#include <thread>
#include <string>

using namespace std;

const int INITIAL_CAPACITY = 10;

struct WordCount {
    string word;
    int count;
};

void resizeWordCounts(WordCount*& arr, int& capacity) {
    int newCapacity = capacity * 2;
    WordCount* newArr = new WordCount[newCapacity];
    for (int i = 0; i < capacity; ++i) {
        newArr[i] = arr[i];
    }
    delete[] arr;
    arr = newArr;
    capacity = newCapacity;
}

char toLower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

void processTextChunk(const string& textChunk, WordCount*& localWordCounts, int& localWordCountSize, int& localWordCountCapacity) {
    string word;
    for (char c : textChunk) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == 39) {
            word += toLower(c);
        } else if (!word.empty()) {
            bool found = false;
            for (int i = 0; i < localWordCountSize; ++i) {
                if (localWordCounts[i].word == word) {
                    localWordCounts[i].count++;
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (localWordCountSize == localWordCountCapacity) {
                    resizeWordCounts(localWordCounts, localWordCountCapacity);
                }
                localWordCounts[localWordCountSize++] = WordCount{word, 1};
            }
            word.clear();
        }
    }
    if (!word.empty()) {
        bool found = false;
        for (int i = 0; i < localWordCountSize; ++i) {
            if (localWordCounts[i].word == word) {
                localWordCounts[i].count++;
                found = true;
                break;
            }
        }
        if (!found) {
            if (localWordCountSize == localWordCountCapacity) {
                resizeWordCounts(localWordCounts, localWordCountCapacity);
            }
            localWordCounts[localWordCountSize++] = WordCount{word, 1};
        }
    }
}

void merge(WordCount arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    WordCount* L = new WordCount[n1];
    WordCount* R = new WordCount[n2];

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

void mergeGlobalCounts(WordCount*& globalCounts, int& globalSize, int& globalCapacity, WordCount* localCounts, int localSize) {
    for (int i = 0; i < localSize; ++i) {
        bool found = false;
        for (int j = 0; j < globalSize; ++j) {
            if (globalCounts[j].word == localCounts[i].word) {
                globalCounts[j].count += localCounts[i].count;
                found = true;
                break;
            }
        }
        if (!found) {
            if (globalSize == globalCapacity) {
                resizeWordCounts(globalCounts, globalCapacity);
            }
            globalCounts[globalSize++] = localCounts[i];
        }
    }
}

int main(int argc, char** argv) {
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

    int numberOfThreads = stoi(argv[2]);
    int maxDepth = 0;
    for (int temp = numberOfThreads; temp > 0; temp >>= 1) {
        maxDepth++;
    }
    maxDepth--; // Adjust because we're counting the number of halvings, not the depth

    thread* threads = new thread[numberOfThreads];
    WordCount** localWordCounts = new WordCount*[numberOfThreads];
    int* localWordCountSizes = new int[numberOfThreads]{0};
    int* localWordCountCapacities = new int[numberOfThreads];

    for (int i = 0; i < numberOfThreads; ++i) {
        localWordCounts[i] = new WordCount[INITIAL_CAPACITY];
        localWordCountCapacities[i] = INITIAL_CAPACITY;
    }

    size_t chunkSize = fileContents.size() / numberOfThreads;
    for (size_t i = 0; i < numberOfThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i + 1) * chunkSize;
        if (i == numberOfThreads - 1) {
            end = fileContents.size();
        }
        threads[i] = thread(processTextChunk, fileContents.substr(start, end - start), ref(localWordCounts[i]), ref(localWordCountSizes[i]), ref(localWordCountCapacities[i]));
    }

    for (int i = 0; i < numberOfThreads; ++i) {
        threads[i].join();
    }

    WordCount* globalWordCounts = new WordCount[INITIAL_CAPACITY];
    int globalWordCountSize = 0;
    int globalWordCountCapacity = INITIAL_CAPACITY;

    for (int i = 0; i < numberOfThreads; ++i) {
        mergeGlobalCounts(globalWordCounts, globalWordCountSize, globalWordCountCapacity, localWordCounts[i], localWordCountSizes[i]);
    }

    mergeSortParallel(globalWordCounts, 0, globalWordCountSize - 1, maxDepth);

    ofstream outputFile("output.txt");
    for (int i = 0; i < globalWordCountSize; ++i) {
        outputFile << globalWordCounts[i].word << " " << globalWordCounts[i].count << "\n";
    }
    outputFile.close();

    for (int i = 0; i < numberOfThreads; ++i) {
        delete[] localWordCounts[i];
    }
    delete[] localWordCounts;
    delete[] localWordCountSizes;
    delete[] localWordCountCapacities;
    delete[] globalWordCounts;
    delete[] threads;

    return 0;
}
