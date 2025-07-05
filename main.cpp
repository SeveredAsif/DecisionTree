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


int main(int argc, char *argv[])
{
    int gainCalculationMethod = 0;
    int maxDepth = 0;
    if (argc >= 3)
    {
        if (strcmp(argv[1], "IG") == 0)
            gainCalculationMethod = 0;
        else if (strcmp(argv[1], "IGR") == 0)
            gainCalculationMethod = 1;
        else if (strcmp(argv[1], "NWIG") == 0)
            gainCalculationMethod = 2;
        else
        {
            cout << "Unknown gain calculation method: " << argv[1] << ". Use IG, IGR, or NWIG.\n";
            return 1;
        }
        maxDepth = atoi(argv[2]);
    }
    else
    {
        cout << "Usage: " << argv[0] << " <gainCalculationMethod> <maxDepth>\n";
        cout << "  gainCalculationMethod: IG, IGR, NWIG\n";
        cout << "  maxDepth: integer\n";
        return 1;
    }
    float totalAccuracy = 0;
    int runs = 1;
    for (int run = 1; run <= runs; ++run)
    {
        // adult.data
        // Iris.csv
        node *root = new node(false, 0);
        std::ifstream file("Datasets/adult.data");
        vector<vector<string>> table;
        if (file.is_open())
        {
            table = readCSV(file);
            file.close();
        }

        unordered_map<string, int> targetColMap;
        int uniqueTargets = 0;
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
                        // isCategoricalColumn[i] = true;
                        // cout << "String error caught" << endl;
                        Preprocessor preprocessor;
                        preprocessor.preprocess(table, i);
                        // cout << "pre process done" << endl;
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
        srand(time(0) + run);
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

        cout << "train set: " << root->getRows().size() << endl;
        cout << "test set: " << testSet->getRows().size() << endl;

        root->setTargetColumn(root->getRows()[0].row.size() - 1);
        testSet->setTargetColumn(root->getRows()[0].row.size() - 1);

        Trainer trainer;
        trainer.train(root, uniqueTargets, maxDepth, gainCalculationMethod, isCategoricalColumn);

        cout << "\nRun " << run << ":\n";

        root->printTree();
        

        // Use the new mixed accuracy function
        float accuracy = testAccuracy(root, testSet);
        cout << "Test set accuracy: " << accuracy * 100 << "%\n";
        totalAccuracy += accuracy;

        delete root;
        delete testSet;
    }

    cout << "\nAverage accuracy over " << runs << " runs: " << (totalAccuracy / runs) * 100 << "%\n";
    return 0;
}