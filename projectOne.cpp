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

// Function to sort the word counts using bubble sort
void sortWordCounts() {
    for (int i = 0; i < wordCountSize - 1; ++i) {
        for (int j = 0; j < wordCountSize - i - 1; ++j) {
            if (wordCounts[j].count < wordCounts[j + 1].count) {
                WordCount temp = wordCounts[j];
                wordCounts[j] = wordCounts[j + 1];
                wordCounts[j + 1] = temp;
            }
        }
    }
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

    std::vector<std::thread> threads;
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
        threads.push_back(std::thread(processTextChunk, chunk));
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> runtime = end - start;

    // Sort the word counts
    sortWordCounts();

    std::cout << "Runtime: " << runtime.count() << std::endl;
    // Write the sorted word counts to the outputFile
    for (int i = 0; i < wordCountSize; ++i) {
        outputFile << wordCounts[i].word << " " << wordCounts[i].count << "\n";
    }
    outputFile.close();

    delete[] wordCounts; // Clean up the dynamic array
    return 0;
}



