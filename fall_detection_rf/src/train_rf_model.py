import pandas as pd
import joblib
import os

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, classification_report

DATA_PATH = "../data/processed/features.csv"
MODEL_PATH = "../models/random_forest_model.pkl"

def main():

    print("Loading feature dataset...")

    df = pd.read_csv(DATA_PATH)

    feature_columns = [
      "ax_mean","ax_std","ax_max","ax_min",
      "ay_mean","ay_std","ay_max","ay_min",
      "az_mean","az_std","az_max","az_min",
      "gx_mean","gx_std","gx_max","gx_min",
      "gy_mean","gy_std","gy_max","gy_min",
      "gz_mean","gz_std","gz_max","gz_min"
    ]

    X = df[feature_columns]
    y = df["label"]

    print("Total samples:", len(df))

    X_train, X_test, y_train, y_test = train_test_split(
        X, y,
        test_size=0.2,
        random_state=42
    )

    print("Training Random Forest...")

    model = RandomForestClassifier(
        n_estimators=20,
        max_depth=8,
        n_jobs=-1,
        random_state=42
    )

    model.fit(X_train, y_train)

    predictions = model.predict(X_test)

    acc = accuracy_score(y_test, predictions)

    print("\nModel Accuracy:", acc)

    print("\nClassification Report:")
    print(classification_report(y_test, predictions))
    

    os.makedirs(os.path.dirname(MODEL_PATH), exist_ok=True)


    joblib.dump(model, MODEL_PATH)

    print("\nModel saved to:", MODEL_PATH)


if __name__ == "__main__":
    main()