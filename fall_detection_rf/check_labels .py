import pandas as pd

df = pd.read_csv("data/processed/features.csv")

print(df["label"].value_counts())