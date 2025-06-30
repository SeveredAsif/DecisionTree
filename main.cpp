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
using namespace std;


//CSV read 
//code source: stack overflow 
//link: https://stackoverflow.com/questions/1120140/how-can-i-read-and-parse-csv-files-in-c
enum class CSVState {
    UnquotedField,
    QuotedField,
    QuotedQuote
};

std::vector<std::string> readCSVRow(const std::string &row) {
    CSVState state = CSVState::UnquotedField;
    std::vector<std::string> fields {""};
    size_t i = 0; // index of the current field
    for (char c : row) {
        switch (state) {
            case CSVState::UnquotedField:
                switch (c) {
                    case ',': // end of field
                              fields.push_back(""); i++;
                              break;
                    case '"': state = CSVState::QuotedField;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedField:
                switch (c) {
                    case '"': state = CSVState::QuotedQuote;
                              break;
                    default:  fields[i].push_back(c);
                              break; }
                break;
            case CSVState::QuotedQuote:
                switch (c) {
                    case ',': // , after closing quote
                              fields.push_back(""); i++;
                              state = CSVState::UnquotedField;
                              break;
                    case '"': // "" -> "
                              fields[i].push_back('"');
                              state = CSVState::QuotedField;
                              break;
                    default:  // end of quote
                              state = CSVState::UnquotedField;
                              break; }
                break;
        }
    }
    return fields;
}

/// Read CSV file, Excel dialect. Accept "quoted fields ""with quotes"""
std::vector<std::vector<std::string>> readCSV(std::istream &in) {
    std::vector<std::vector<std::string>> table;
    std::string row;
    while (!in.eof()) {
        std::getline(in, row);
        if (in.bad() || in.fail()) {
            break;
        }
        auto fields = readCSVRow(row);
        table.push_back(fields);
    }
    return table;
}
int main()
{
    node* root = new node(false,0);
    // Row *r = new Row();
    // root->addRow(*r);
    //cout<<root->getRows()[0].row[0]<<endl;
    std::ifstream file("Datasets/Iris.csv");
    vector<vector<string>> table;
    if (file.is_open()) {
        table = readCSV(file);
        // Now 'table' contains the CSV data as a vector of vector of strings
        file.close();

    }
    // for(auto val: table[1]){
    //         cout<<val<<",";
    // }
    unordered_map<string, int> targetColMap;
    int uniqueTargets = 0;
    int totalColumns = table[0].size();
    for(int id=1;id<table.size();id++ ){
        auto &rows = table[id];
        if(targetColMap.find(rows[totalColumns-1])==targetColMap.end()){
            //find out if the target column already converted into a number
            targetColMap[rows[totalColumns-1]] = uniqueTargets++;
        }
        //populate my rows class object
        Row currentRow;
        for(int i=0;i<totalColumns;i++){
            if(i!=totalColumns-1){
                //cout<<rows[i]<<endl;
                float str_float = stof(rows[i]);
                currentRow.insertColumn(str_float);
            }
            else{
                currentRow.insertColumn(targetColMap[rows[i]]);
                //cout<<targetColMap[rows[i]]<<endl;
            }
        }
        root->addRow(currentRow);

    }
    //root->print();

    //train-test split
    node * testSet = new node(false,0);
    srand(time(0));
    int rootSize = root->getRows().size();
    cout<<"Current root size: "<<rootSize<<endl;
    for(int i=0;i<(int)rootSize*0.2;i++){
        int index = rand() % root->getRows().size();
        //cout<<index<<endl;
        testSet->addRow(root->getRows()[index]);
        if (index >= 0 && index < root->getRows().size()) {
            root->getRows().erase(root->getRows().begin() + index);
        }
    }
    cout<<"train set: "<<root->getRows().size()<<endl;
    cout<<"test set: "<<testSet->getRows().size()<<endl;
    
    //cout<<table[0][table[0].size()-1]<<endl;
    root->setTargetColumn(root->getRows()[0].row.size()-1);
    testSet->setTargetColumn(root->getRows()[0].row.size()-1);
    //root->print();
    Trainer trainer;
    trainer.train(root,uniqueTargets);
    root->printTree();
}