#include "row.hpp"
#include "node.hpp"
#include "trainer.hpp"
#include "preprocessor.hpp"
#include <iostream>
#include <istream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <map>
#include <sstream>
#include <stdlib.h>
#include <ctime>
#include <cstring>
#include <algorithm>
#include <random>
#include <chrono>
using namespace std;

// Global variable for categorical columns (used in main.cpp)
vector<bool> isCategoricalColumn;

// CSV read functions from main.cpp
enum class CSVState
{
    UnquotedField,
    QuotedField,
    QuotedQuote
};

std::vector<std::string> readCSVRow(const std::string &row)
{
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields{""};
    size_t i = 0; // index of the current field
    for (char c : row)
    {
        switch (state)
        {
        case CSVState::UnquotedField:
            switch (c)
            {
            case ',': // end of field
                fields.push_back("");
                i++;
                break;
            case '"':
                state = CSVState::QuotedField;
                break;
            default:
                fields[i].push_back(c);
                break;
            }
            break;
        case CSVState::QuotedField:
            switch (c)
            {
            case '"':
                state = CSVState::QuotedQuote;
                break;
            default:
                fields[i].push_back(c);
                break;
            }
            break;
        case CSVState::QuotedQuote:
            switch (c)
            {
            case ',': // , after closing quote
                fields.push_back("");
                i++;
                state = CSVState::UnquotedField;
                break;
            case '"': // "" -> "
                fields[i].push_back('"');
                state = CSVState::QuotedField;
                break;
            default: // end of quote
                state = CSVState::UnquotedField;
                break;
            }
            break;
        }
    }
    return fields;
}

std::vector<std::vector<std::string>> readCSV(std::istream &in)
{
    std::vector<std::vector<std::string>> table;
    std::string row;
    while (!in.eof())
    {
        std::getline(in, row);
        if (in.bad() || in.fail())
        {
            break;
        }
        auto fields = readCSVRow(row);
        table.push_back(fields);
    }
    return table;
}

// Test accuracy function from main.cpp
float testAccuracy(node *root, node *testSet)
{
    int correct = 0;
    int total = testSet->getRows().size();
    int targetCol = root->getTargetColumn();

    for (const auto &testRow : testSet->getRows())
    {
        node *curr = root;
        int level = 0;

        while (!curr->getIsLeaf())
        {
            int splitCol = curr->getSplitCol();
            float val = testRow.row[splitCol];

            if (splitCol < isCategoricalColumn.size() && isCategoricalColumn[splitCol])
            {
                const auto &catMap = curr->getCategoryChildMap();
                auto it = catMap.find(val);
                if (it != catMap.end() && it->second < curr->getChildren().size())
                {
                    curr = curr->getChildren()[it->second];
                }
                else
                {
                    break;
                }
            }
            else
            {
                float splitVal = curr->getSplitval();
                if (val <= splitVal)
                {
                    if (curr->getChildren().size() > 0)
                        curr = curr->getChildren()[0]; // left child
                    else
                        break;
                }
                else
                {
                    if (curr->getChildren().size() > 1)
                        curr = curr->getChildren()[1]; // right child
                    else
                        break;
                }
            }
            level++;
        }

        if (!curr->getRows().empty() && curr->getRows()[0].row.size() > targetCol)
        {
            std::map<int, int> labelCounts;
            for (const auto &r : curr->getRows())
            {
                int label = r.row[targetCol];
                labelCounts[label]++;
            }

            int predicted = -1, maxCount = -1;
            for (const auto &kv : labelCounts)
            {
                if (kv.second > maxCount)
                {
                    maxCount = kv.second;
                    predicted = kv.first;
                }
            }

            int actual = testRow.row[targetCol];
            if (predicted == actual)
                correct++;
        }
    }

    return (float)correct / total;
}

// Function to count nodes in the tree
int countNodes(node *root)
{
    if (!root)
        return 0;

    int count = 1; // Count the current node
    for (auto child : root->getChildren())
    {
        count += countNodes(child);
    }
    return count;
}

// Function to get maximum depth of the tree
int getTreeDepth(node *root)
{
    if (!root || root->getIsLeaf())
        return 0;

    int maxDepth = 0;
    for (auto child : root->getChildren())
    {
        maxDepth = max(maxDepth, getTreeDepth(child));
    }
    return maxDepth + 1;
}

// Run experiments and generate reports for a single dataset
void runExperiments(const string &datasetPath, const string &datasetName)
{
    cout << "Running experiments for dataset: " << datasetName << endl;

    // Create result containers
    map<string, vector<float>> accuracyByMethod;  // Method -> [accuracy for each depth]
    map<string, vector<int>> nodesByMethod;       // Method -> [nodes for each depth]
    map<string, vector<int>> actualDepthByMethod; // Method -> [actual depth for each depth]

    vector<string> methods = {"IG", "IGR", "NWIG"};
    vector<int> depths = { 2, 3, 4, 5,0}; // 0 means no pruning
    int numExperiments = 20;              // Run 20 experiments for each configuration

    // Initialize result containers
    for (const auto &method : methods)
    {
        accuracyByMethod[method] = vector<float>(depths.size(), 0.0);
        nodesByMethod[method] = vector<int>(depths.size(), 0);
        actualDepthByMethod[method] = vector<int>(depths.size(), 0);
    }

    // Load the dataset once
    std::ifstream file(datasetPath);
    vector<vector<string>> table;
    if (file.is_open())
    {
        table = readCSV(file);
        file.close();
    }
    else
    {
        cout << "Error: Could not open dataset " << datasetPath << endl;
        return;
    }

    // Write accuracy, node count, and actual depth results to CSV after each run
    ofstream accuracyFile(datasetName + "_accuracy5JUL.csv");
    ofstream nodesFile(datasetName + "_nodes5JUL.csv");
    ofstream actualDepthFile(datasetName + "_actualDepth5JUL.csv");
    ofstream eachRunInfo(datasetName+"eachRunInfo5JUL.txt", ios::app);
    if (accuracyFile.is_open())
    {
        accuracyFile << "Depth,";
        for (const auto &method : methods)
        {
            accuracyFile << method << ",";
        }
        accuracyFile << endl;
    }
    if (nodesFile.is_open())
    {
        nodesFile << "Depth,";
        for (const auto &method : methods)
        {
            nodesFile << method << ",";
        }
        nodesFile << endl;
    }
    if (actualDepthFile.is_open())
    {
        actualDepthFile << "Depth,";
        for (const auto &method : methods)
        {
            actualDepthFile << method << ",";
        }
        actualDepthFile << endl;
    }
    // Process each method and depth combination
    for (const auto &method : methods)
    {
        cout << "  Processing method: " << method << endl;
        int gainCalculationMethod;
        if (method == "IG")
            gainCalculationMethod = 0;
        else if (method == "IGR")
            gainCalculationMethod = 1;
        else
            gainCalculationMethod = 2; // NWIG
        auto methodStart = chrono::high_resolution_clock::now();
        for (size_t d = 0; d < depths.size(); d++)
        {
            int depth = depths[d];
            cout << "    Processing depth: " << depth << endl;
            float totalAccuracy = 0.0;
            int totalNodes = 0;
            int totalActualDepth = 0;
            auto depthStart = chrono::high_resolution_clock::now();
            for (int exp = 0; exp < numExperiments; exp++)
            {
                auto expStart = chrono::high_resolution_clock::now();
                // Create a fresh copy of the dataset for each experiment
                unordered_map<string, int> targetColMap;
                int uniqueTargets = 0;
                int totalColumns = table[0].size();

                // Reset categorical column flags
                isCategoricalColumn.clear();
                isCategoricalColumn.resize(totalColumns, false);

                node *root = new node(false, 0);

                // Process all rows
                for (int id = 1; id < table.size(); id++)
                {
                    auto &rows = table[id];
                    if (rows.size() < totalColumns)
                        continue; // Skip incomplete rows

                    if (targetColMap.find(rows[totalColumns - 1]) == targetColMap.end())
                    {
                        targetColMap[rows[totalColumns - 1]] = uniqueTargets++;
                    }

                    Row currentRow;
                    for (int i = 0; i < totalColumns; i++)
                    {
                        if (i != totalColumns - 1)
                        {
                            try
                            {
                                stof(rows[i]);
                            }
                            catch (invalid_argument &e)
                            {
                                // Handle categorical data
                                Preprocessor preprocessor;
                                preprocessor.preprocess(table, i);
                                isCategoricalColumn[i] = true;
                            }

                            float str_float = stof(rows[i]);
                            currentRow.insertColumn(str_float);
                        }
                        else
                        {
                            currentRow.insertColumn(targetColMap[rows[i]]);
                        }
                    }
                    root->addRow(currentRow);
                }

                // Create 80/20 split with a different seed for each experiment
                unsigned seed = static_cast<unsigned>(time(nullptr)) + exp;
                std::mt19937 generator(seed);

                node *testSet = new node(false, 0);
                vector<int> indices(root->getRows().size());
                for (size_t i = 0; i < indices.size(); i++)
                {
                    indices[i] = i;
                }

                // Shuffle indices
                shuffle(indices.begin(), indices.end(), generator);

                // Select 20% for testing
                int testSize = root->getRows().size() * 0.2;
                for (int i = 0; i < testSize; i++)
                {
                    testSet->addRow(root->getRows()[indices[i]]);
                }

                // Remove test indices from root (in reverse order to avoid index issues)
                sort(indices.begin(), indices.begin() + testSize, greater<int>());
                for (int i = 0; i < testSize; i++)
                {
                    if (indices[i] < root->getRows().size())
                    {
                        root->getRows().erase(root->getRows().begin() + indices[i]);
                    }
                }

                // Set target column
                if (!root->getRows().empty() && !testSet->getRows().empty())
                {
                    root->setTargetColumn(root->getRows()[0].row.size() - 1);
                    testSet->setTargetColumn(testSet->getRows()[0].row.size() - 1);

                    // Train the decision tree
                    Trainer trainer;
                    trainer.train(root, uniqueTargets, depth, gainCalculationMethod, isCategoricalColumn);

                    // Test accuracy
                    float accuracy = testAccuracy(root, testSet);
                    int nodeCount = countNodes(root);
                    int actualDepth = getTreeDepth(root);

                    totalAccuracy += accuracy;
                    totalNodes += nodeCount;
                    totalActualDepth += actualDepth;
                    // Write progress to console and CSV after each run
                    auto expEnd = chrono::high_resolution_clock::now();
                    double expTime = chrono::duration<double>(expEnd - expStart).count();
                    string runMsg = "      Run " + to_string(exp + 1) + "/" + to_string(numExperiments) + ": Accuracy = " + to_string(accuracy * 100) + "% , Nodes = " + to_string(nodeCount) + ", ActualDepth = " + to_string(actualDepth) + ", Time = " + to_string(expTime) + "s\n";
                    cout << runMsg;
                    if (eachRunInfo.is_open())
                        eachRunInfo << "Method: " << method << ", Depth: " << depth << ", " << runMsg;
                }

                // Clean up
                delete root;
                delete testSet;
            }
            auto depthEnd = chrono::high_resolution_clock::now();
            double depthTime = chrono::duration<double>(depthEnd - depthStart).count();
            string depthMsg = "    Finished depth " + to_string(depth) + " in " + to_string(depthTime) + "s\n";
            cout << depthMsg;
            if (eachRunInfo.is_open())
                eachRunInfo << "Method: " << method << ", Depth: " << depth << ", " << depthMsg;
            // Write newline after each depth for CSV
            if (accuracyFile.is_open())
                accuracyFile << endl;
            if (nodesFile.is_open())
                nodesFile << endl;
            if (actualDepthFile.is_open())
                actualDepthFile << endl;
            // Calculate averages
            accuracyByMethod[method][d] = totalAccuracy / numExperiments;
            nodesByMethod[method][d] = totalNodes / numExperiments;
            actualDepthByMethod[method][d] = totalActualDepth / numExperiments;
        }
        auto methodEnd = chrono::high_resolution_clock::now();
        double methodTime = chrono::duration<double>(methodEnd - methodStart).count();
        string methodMsg = "  Finished method " + method + " in " + to_string(methodTime) + "s\n";
        cout << methodMsg;
        if (eachRunInfo.is_open())
            eachRunInfo << methodMsg;
    }
    if (eachRunInfo.is_open())
        eachRunInfo.close();
    // Write averages to CSV at the end
    if (accuracyFile.is_open())
    {
        for (size_t d = 0; d < depths.size(); d++)
        {
            if (depths[d] == 0)
                accuracyFile << "NoPrune,";
            else
                accuracyFile << depths[d] << ",";
            for (const auto &method : methods)
            {
                accuracyFile << accuracyByMethod[method][d] * 100 << ",";
            }
            accuracyFile << endl;
        }
    }
    if (nodesFile.is_open())
    {
        for (size_t d = 0; d < depths.size(); d++)
        {
            if (depths[d] == 0)
                nodesFile << "NoPrune,";
            else
                nodesFile << depths[d] << ",";
            for (const auto &method : methods)
            {
                nodesFile << nodesByMethod[method][d] << ",";
            }
            nodesFile << endl;
        }
    }
    if (actualDepthFile.is_open())
    {
        for (size_t d = 0; d < depths.size(); d++)
        {
            if (depths[d] == 0)
                actualDepthFile << "NoPrune,";
            else
                actualDepthFile << depths[d] << ",";
            for (const auto &method : methods)
            {
                actualDepthFile << actualDepthByMethod[method][d] << ",";
            }
            actualDepthFile << endl;
        }
    }
}

int main()
{
    // Process both datasets
    cout << "Starting experiment runs..." << endl;

    // Run experiments for adult.data
    // runExperiments("Datasets/Iris.csv", "Iris");

    // Run experiments for Iris.csv
    runExperiments("Datasets/adult.data", "Adult");

    cout << "All experiments completed." << endl;
    cout << "CSV files generated for plotting:" << endl;
    cout << "  - Adult_accuracyWEIRD.csv" << endl;
    cout << "  - Adult_nodesWEIRD.csv" << endl;
    cout << "  - Iris_accuracyWEIRD.csv" << endl;
    cout << "  - Iris_nodesWEIRD.csv" << endl;

    return 0;
}