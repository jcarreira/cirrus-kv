import csv

# preprocess script for the adult.data file (https://archive.ics.uci.edu/ml/datasets/Adult)

# we only keep numerical variables
# we should handle categorical but I don't have time now

# numerical variable indexes (from 0)
# 0, 2, 4, 10, 11, 12
# label is in 14

with open('adult.data', 'rb') as csvfile:
    reader = csv.reader(csvfile, delimiter=',')
    for row in reader:
        line = [row[0], row[2], row[4], row[10], row[11], row[12]]
        label = ""
        
        if not row[14][0].isalpha():
            row[14] = row[14][1:]
        if row[14] == '<=50K':
            label = '1'
        else:
            label = '0'

        line.append(label)
        print ','.join(line)

