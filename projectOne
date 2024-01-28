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
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            word += toLower(c);
        } else if (!word.empty()) {
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
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <inputFile> <outputFile>\n";
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

    // For simplicity, the text is processed in a single thread in this example
    processTextChunk(fileContents);

    // Sort the word counts
    sortWordCounts();

    std::ofstream outputFile(argv[2]);
    for (int i = 0; i < wordCountSize; ++i) {
        outputFile << wordCounts[i].word << " " << wordCounts[i].count << "\n";
    }

    delete[] wordCounts; // Clean up the dynamic array
    return 0;
}


