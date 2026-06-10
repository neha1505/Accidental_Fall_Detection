import pandas as pd
import joblib

df = pd.read_csv("data/processed/features.csv")

model = joblib.load("models/random_forest_model.pkl")

X = df.drop("label", axis=1)

pred = model.predict(X)

print("Unique predictions:", set(pred))