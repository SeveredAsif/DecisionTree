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
#include "preprocessor.hpp"
using namespace std;

int isCategorical = 0;

// CSV read
// code source: stack overflow
// link: https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
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

/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
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
        std::vector<float> pathSplits;
        std::vector<int> pathCols;
        std::vector<bool> wentLeft;
        // std::cout << "\033[36mTest row: ";
        // testRow.print();
        // std::cout << "\033[0m";
        while (!curr->getIsLeaf())
        {
            int splitCol = curr->getSplitCol();
            float splitVal = curr->getSplitval();
            float val = testRow.row[splitCol];
            // std::cout << "  Level " << level << ": Feature[" << splitCol << "] = " << val;
            if (val <= splitVal)
            {
                // std::cout << " \033[32m<=\033[0m " << splitVal << " -- go LEFT\n";
                if (curr->getChildren().size() > 0)
                    curr = curr->getChildren()[0]; // left child
                else
                    break;
                wentLeft.push_back(true);
            }
            else
            {
                // std::cout << " \033[31m>\033[0m " << splitVal << " -- go RIGHT\n";
                if (curr->getChildren().size() > 1)
                    curr = curr->getChildren()[1]; // right child
                else
                    break;
                wentLeft.push_back(false);
            }
            pathSplits.push_back(splitVal);
            pathCols.push_back(splitCol);
            level++;
        }
        // At leaf, predict the majority label
        if (!curr->getRows().empty() && curr->getRows()[0].row.size() > targetCol)
        {
            std::map<int, int> labelCounts;
            float maxSplitVal = -1e9;
            for (const auto &r : curr->getRows())
            {
                int label = r.row[targetCol];
                labelCounts[label]++;
                // Find max split value in this leaf for debug
                for (size_t i = 0; i < r.row.size(); ++i)
                {
                    if (r.row[i] > maxSplitVal)
                        maxSplitVal = r.row[i];
                }
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
            // std::cout << "  \033[35mReached leaf. Max value in leaf: " << maxSplitVal << ". Predicting: " << predicted << ", Actual: " << actual << "\033[0m\n";
            if (predicted == actual)
                correct++;
        }
        // std::cout << "\033[90m-----------------------------\033[0m\n";
    }
    return (float)correct / total;
}
float testAccuracyCategorical(node *root, node *testSet)
{
    int correct = 0;
    int total = testSet->getRows().size();
    int targetCol = root->getTargetColumn();
    for (const auto &testRow : testSet->getRows())
    {
        node *curr = root;
        while (!curr->getIsLeaf())
        {
            int splitCol = curr->getSplitCol();
            float val = testRow.row[splitCol];
            const auto &catMap = curr->getCategoryChildMap();
            auto it = catMap.find(val);
            if (it != catMap.end() && it->second < curr->getChildren().size())
            {
                curr = curr->getChildren()[it->second];
            }
            else
            {
                break; // Unseen category, fallback to majority at this node
            }
        }
        // At leaf, predict the majority label
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
int main()
{
    vector<int> depths = {2, 3, 4};
    int runs = 20;
    string dataset = "Datasets/adult.data"; // You can change this to "Datasets/Iris.csv" or others
    ofstream report("experiment_results.txt", ios::app);
    if (!report.is_open())
    {
        cout << "Failed to open experiment_results.txt for writing.\n";
        return 1;
    }
    report << "\n==== Experiment on " << dataset << " ====" << endl;
    for (int maxDepth : depths)
    {
        float totalAccuracy = 0;
        report << "\nDepth: " << maxDepth << endl;
        for (int run = 1; run <= runs; ++run)
        {
            node *root = new node(false, 0);
            std::ifstream file(dataset);
            vector<vector<string>> table;
            if (file.is_open())
            {
                table = readCSV(file);
                file.close();
            }
            unordered_map<string, int> targetColMap;
            int uniqueTargets = 0;
            int totalColumns = table[0].size();
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
                            isCategorical = 0;
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
            node *testSet = new node(false, 0);
            srand(time(0) + run + maxDepth * 100);
            int rootSize = root->getRows().size();
            for (int i = 0; i < (int)rootSize * 0.2; i++)
            {
                int index = rand() % root->getRows().size();
                testSet->addRow(root->getRows()[index]);
                if (index >= 0 && index < root->getRows().size())
                {
                    root->getRows().erase(root->getRows().begin() + index);
                }
            }
            root->setTargetColumn(root->getRows()[0].row.size() - 1);
            testSet->setTargetColumn(root->getRows()[0].row.size() - 1);
            Trainer trainer;
            trainer.train(root, uniqueTargets, maxDepth, 2);
            float accuracy = 0.0f;
            if (isCategorical)
                accuracy = testAccuracyCategorical(root, testSet);
            else
                accuracy = testAccuracy(root, testSet);
            totalAccuracy += accuracy;
            // Print tree to file (append mode)
            std::ofstream treeOut("tree_prints.txt", ios::app);
            if (treeOut.is_open())
            {
                treeOut << "\nRun " << run << " (Depth " << maxDepth << "):\n";
                root->printTreeToFile(treeOut);
                treeOut.close();
            }
            report << "Run " << run << ": " << accuracy * 100 << "%\n";
            delete root;
            delete testSet;
        }
        report << "Average accuracy for depth " << maxDepth << ": " << (totalAccuracy / runs) * 100 << "%\n";
        report.flush();
    }
    report << "==== End of experiment ====\n";
    report.close();
    cout << "Experiment completed. Results written to experiment_results.txt\n";
    return 0;
}