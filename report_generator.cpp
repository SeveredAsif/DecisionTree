#include "row.hpp"
#include "node.hpp"
#include "trainer.hpp"
#include <iostream>
#include <istream>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <stdlib.h>
#include <ctime>
#include <cstring>
#include "preprocessor.hpp"
using namespace std;

vector<bool> isCategoricalColumn;

// CSV read functions (same as main.cpp)
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
    size_t i = 0;
    for (char c : row)
    {
        switch (state)
        {
        case CSVState::UnquotedField:
            switch (c)
            {
            case ',':
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
            case ',':
                fields.push_back("");
                i++;
                state = CSVState::UnquotedField;
                break;
            case '"':
                fields[i].push_back('"');
                state = CSVState::QuotedField;
                break;
            default:
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
                        curr = curr->getChildren()[0];
                    else
                        break;
                }
                else
                {
                    if (curr->getChildren().size() > 1)
                        curr = curr->getChildren()[1];
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

    int count = 1; // Count current node
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

// Function to load and preprocess dataset
node *loadDataset(const string &filename, unordered_map<string, int> &targetColMap, int &uniqueTargets)
{
    node *root = new node(false, 0);
    std::ifstream file(filename);
    vector<vector<string>> table;

    if (file.is_open())
    {
        table = readCSV(file);
        file.close();
    }
    else
    {
        cout << "Error: Could not open file " << filename << endl;
        return nullptr;
    }

    targetColMap.clear();
    uniqueTargets = 0;
    int totalColumns = table[0].size();

    isCategoricalColumn.clear();
    isCategoricalColumn.resize(totalColumns, false);

    for (int id = 1; id < table.size(); id++)
    {
        auto &rows = table[id];
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
                    Preprocessor preprocessor;
                    preprocessor.preprocess(table, i);
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

    return root;
}

// Function to create train/test split
pair<node *, node *> createTrainTestSplit(node *dataset, double testRatio = 0.2, int seed = 42)
{
    node *trainSet = new node(false, 0);
    node *testSet = new node(false, 0);

    // Copy all rows to train set first
    for (const Row &row : dataset->getRows())
    {
        trainSet->addRow(const_cast<Row &>(row));
    }

    srand(seed);
    int totalSize = trainSet->getRows().size();
    int testSize = (int)(totalSize * testRatio);

    // Move random rows from train to test
    for (int i = 0; i < testSize; i++)
    {
        int index = rand() % trainSet->getRows().size();
        testSet->addRow(trainSet->getRows()[index]);
        trainSet->getRows().erase(trainSet->getRows().begin() + index);
    }

    // Set target columns
    int targetCol = dataset->getRows()[0].row.size() - 1;
    trainSet->setTargetColumn(targetCol);
    testSet->setTargetColumn(targetCol);

    return make_pair(trainSet, testSet);
}

// Function to run experiment for a specific configuration
struct ExperimentResult
{
    int depth;
    string method;
    string dataset;
    float accuracy;
    int nodeCount;
    int actualDepth;
};

ExperimentResult runExperiment(const string &datasetFile, const string &datasetName,
                               int maxDepth, int gainMethod, const string &methodName,
                               int runs = 5)
{
    ExperimentResult result;
    result.depth = maxDepth;
    result.method = methodName;
    result.dataset = datasetName;
    result.accuracy = 0;
    result.nodeCount = 0;
    result.actualDepth = 0;

    float totalAccuracy = 0;
    int totalNodes = 0;
    int totalActualDepth = 0;

    for (int run = 0; run < runs; run++)
    {
        unordered_map<string, int> targetColMap;
        int uniqueTargets = 0;

        // Load dataset
        node *dataset = loadDataset(datasetFile, targetColMap, uniqueTargets);
        if (!dataset)
            continue;

        // Create train/test split
        auto splitResult = createTrainTestSplit(dataset, 0.2, 42 + run);
        node *trainSet = splitResult.first;
        node *testSet = splitResult.second;

        // Train the model
        Trainer trainer;
        trainer.train(trainSet, uniqueTargets, maxDepth, gainMethod, isCategoricalColumn);

        // Calculate metrics
        float accuracy = testAccuracy(trainSet, testSet);
        int nodeCount = countNodes(trainSet);
        int actualDepth = getTreeDepth(trainSet);

        totalAccuracy += accuracy;
        totalNodes += nodeCount;
        totalActualDepth += actualDepth;

        // Cleanup
        delete dataset;
        delete trainSet;
        delete testSet;
    }

    result.accuracy = totalAccuracy / runs;
    result.nodeCount = totalNodes / runs;
    result.actualDepth = totalActualDepth / runs;

    return result;
}

int main()
{
    vector<string> datasets = {"Datasets/Iris.csv", "Datasets/adult.data"};
    vector<string> datasetNames = {"Iris", "Adult"};
    vector<string> methods = {"IG", "IGR", "NWIG"};
    vector<int> methodCodes = {0, 1, 2};
    vector<int> depths = {0};

    vector<ExperimentResult> results;

    cout << "Starting comprehensive decision tree analysis..." << endl;
    cout << "Testing " << datasets.size() << " datasets, " << methods.size()
         << " methods, and " << depths.size() << " depth levels." << endl;

    int totalExperiments = datasets.size() * methods.size() * depths.size();
    int currentExperiment = 0;

    // Run all experiments
    for (int d = 0; d < datasets.size(); d++)
    {
        for (int m = 0; m < methods.size(); m++)
        {
            for (int depth : depths)
            {
                currentExperiment++;
                cout << "Running experiment " << currentExperiment << "/" << totalExperiments
                     << " - Dataset: " << datasetNames[d] << ", Method: " << methods[m]
                     << ", Depth: " << depth << endl;

                ExperimentResult result = runExperiment(datasets[d], datasetNames[d],
                                                        depth, methodCodes[m], methods[m]);
                results.push_back(result);

                cout << "  Accuracy: " << result.accuracy * 100 << "%, Nodes: "
                     << result.nodeCount << ", Actual Depth: " << result.actualDepth << endl;
            }
        }
    }

    // Generate CSV files

    // 1. Accuracy vs Max Depth for all combinations
    ofstream accuracyFile("accuracy_vs_depth_no_prune.csv");
    accuracyFile << "Dataset,Method,MaxDepth,Accuracy\n";
    for (const auto &result : results)
    {
        accuracyFile << result.dataset << "," << result.method << ","
                     << result.depth << "," << result.accuracy * 100 << "\n";
    }
    accuracyFile.close();

    // 2. Node Count vs Max Depth for all combinations
    ofstream nodesFile("nodes_vs_depth_no_prune.csv");
    nodesFile << "Dataset,Method,MaxDepth,NodeCount,ActualDepth\n";
    for (const auto &result : results)
    {
        nodesFile << result.dataset << "," << result.method << ","
                  << result.depth << "," << result.nodeCount << ","
                  << result.actualDepth << "\n";
    }
    nodesFile.close();

    // 3. Separate files for each dataset
    for (const string &datasetName : datasetNames)
    {
        string filename = datasetName + "_analysis_no_prune.csv";
        ofstream datasetFile(filename);
        datasetFile << "Method,MaxDepth,Accuracy,NodeCount,ActualDepth\n";

        for (const auto &result : results)
        {
            if (result.dataset == datasetName)
            {
                datasetFile << result.method << "," << result.depth << ","
                            << result.accuracy * 100 << "," << result.nodeCount << ","
                            << result.actualDepth << "\n";
            }
        }
        datasetFile.close();
    }

    // 4. Summary statistics
    ofstream summaryFile("summary_statistics_no_prune.csv");
    summaryFile << "Dataset,Method,BestDepth,BestAccuracy,AvgAccuracy,MaxNodes,AvgNodes\n";

    for (const string &datasetName : datasetNames)
    {
        for (const string &method : methods)
        {
            float bestAccuracy = 0;
            int bestDepth = 0;
            float totalAccuracy = 0;
            int totalNodes = 0;
            int maxNodes = 0;
            int count = 0;

            for (const auto &result : results)
            {
                if (result.dataset == datasetName && result.method == method)
                {
                    if (result.accuracy > bestAccuracy)
                    {
                        bestAccuracy = result.accuracy;
                        bestDepth = result.depth;
                    }
                    totalAccuracy += result.accuracy;
                    totalNodes += result.nodeCount;
                    maxNodes = max(maxNodes, result.nodeCount);
                    count++;
                }
            }

            summaryFile << datasetName << "," << method << "," << bestDepth << ","
                        << bestAccuracy * 100 << "," << (totalAccuracy / count) * 100 << ","
                        << maxNodes << "," << totalNodes / count << "\n";
        }
    }
    summaryFile.close();

    cout << "\nAnalysis complete! Generated files:" << endl;
    cout << "- accuracy_vs_depth.csv: Accuracy vs maximum depth for all combinations" << endl;
    cout << "- nodes_vs_depth.csv: Node count vs maximum depth for all combinations" << endl;
    cout << "- Adult_analysis.csv: Detailed analysis for Adult dataset" << endl;
    cout << "- Iris_analysis.csv: Detailed analysis for Iris dataset" << endl;
    cout << "- summary_statistics.csv: Summary statistics and best configurations" << endl;

    return 0;
}