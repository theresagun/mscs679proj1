#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>

const int INITIAL_CAPACITY = 10; // Initial size of the dynamic array

struct WordCount {
    std::string word;
    int count;
};

void quickSort(WordCount* arr, int low, int high);
int partition(WordCount* arr, int low, int high);

WordCount* wordCounts = new WordCount[INITIAL_CAPACITY];
int wordCountSize = 0;
int wordCountCapacity = INITIAL_CAPACITY;
std::mutex wordCountMutex;

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
void processTextChunk(const std::string& textChunk) {
    std::string word;
    for (char c : textChunk) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '-' || c == 39) {
            // buildup a word with letters, hyphens, and apostrophes
            word += toLower(c);
        } else if (!word.empty()) {
            // if there is a space, number or special character then that's the end of the word
            std::lock_guard<std::mutex> guard(wordCountMutex);
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
                wordCounts[wordCountSize++] = {word, 1};
            }

            word.clear();
        }
    }

    if (!word.empty()) {
        // handle the last word in the chunk if it wasn't already handled
        std::lock_guard<std::mutex> guard(wordCountMutex);
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
            wordCounts[wordCountSize++] = {word, 1};
        }
    }
}

//function to do quicksort method
void quickSort(WordCount* arr, int low, int high) {
    if (low < high) {
        // Partition the array
        int pivotIndex = partition(arr, low, high);

        // Recursively sort the subarrays
        quickSort(arr, low, pivotIndex - 1);
        quickSort(arr, pivotIndex + 1, high);
    }
}

// Function to find the partition position
int partition(WordCount* arr, int low, int high) {
    WordCount pivot = arr[high];
    int i = (low - 1);

    for (int j = low; j < high; j++) {
        if (arr[j].count > pivot.count) {
            i++;
            // Swap arr[i] and arr[j]
            WordCount temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    // Swap arr[i + 1] and arr[high] (or pivot)
    WordCount temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;

    return i + 1;
}

int main(int argc, char** argv) {
    std::cout << "Program Starting" << std::endl;
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <inputFile> <numberOfThreads>\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::ifstream file(inputFile);
    if (!file) {
        std::cerr << "Error opening file: " << inputFile << "\n";
        return 1;
    }

    std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    int numThreads = std::atoi(argv[2]);
    std::ofstream outputFile("output.txt");

    auto start = std::chrono::high_resolution_clock::now();

    std::thread* threads = new std::thread[numThreads];
    size_t chunkSize = fileContents.size() / numThreads;

    for (size_t i = 0; i < numThreads; ++i) {
        size_t start = i * chunkSize;
        size_t end = (i + 1) * chunkSize;

        if (i == numThreads - 1) {
            end = fileContents.size();
        }

        // Ensure we don't split words between chunks
        if (end < fileContents.size()) {
        while (end < fileContents.size() && (std::isalpha(fileContents[end]) || fileContents[end] == '-' || fileContents[end] == 39)) {
                end++;
            }
        }

        std::string chunk = fileContents.substr(start, end - start);
        threads[i] = std::thread(processTextChunk, chunk);
    }

    // Wait for all threads to finish
    for (size_t i = 0; i < numThreads; ++i) {
        threads[i].join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> runtime = end - start;

    // Sort the word counts
    quickSort(wordCounts, 0, wordCountSize - 1);

    std::cout << "Runtime: " << runtime.count() << std::endl;
    // Write the sorted word counts to the outputFile
    for (int i = 0; i < wordCountSize; ++i) {
        outputFile << wordCounts[i].word << " " << wordCounts[i].count << "\n";
    }
    outputFile.close();
    delete[] threads;

    delete[] wordCounts; // Clean up the dynamic array
    return 0;
}



