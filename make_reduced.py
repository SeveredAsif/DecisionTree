import random

# Read all lines from adult.data
with open('Datasets/adult.data', 'r') as f:
    lines = [line for line in f if line.strip()]

# Take a random sample of 10,000 lines
sample = random.sample(lines, 8000)

# Write to reduced.csv
with open('Datasets/reduced.csv', 'w') as f:
    f.writelines(sample)

print('Reduced dataset saved to Datasets/reduced.csv')
