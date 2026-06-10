import pandas as pd
import numpy as np

INPUT_FILE = "../data/processed/labeled_sensor_data.csv"
OUTPUT_FILE = "../data/processed/features.csv"

WINDOW_SIZE = 200

features = []


def scale_sensor_data(df):

    # same scaling as ESP32 firmware
    df["ax"] = df["ax"] / 16384
    df["ay"] = df["ay"] / 16384
    df["az"] = df["az"] / 16384

    df["gx"] = df["gx"] / 131
    df["gy"] = df["gy"] / 131
    df["gz"] = df["gz"] / 131

    return df


def extract_features(window):

    feature_vector = []

    for col in ["ax","ay","az","gx","gy","gz"]:

        data = window[col]

        feature_vector.append(data.mean())
        feature_vector.append(data.std())
        feature_vector.append(data.max())
        feature_vector.append(data.min())

    return feature_vector


def main():

    print("Loading labeled dataset...")

    df = pd.read_csv(INPUT_FILE)

    # scale values
    df = scale_sensor_data(df)

    print("Total samples:", len(df))

    for i in range(0, len(df) - WINDOW_SIZE, WINDOW_SIZE):

        window = df.iloc[i:i+WINDOW_SIZE]

        label = window["label"].mode()[0]

        feats = extract_features(window)

        feats.append(label)

        features.append(feats)

    columns = []

    sensors = ["ax","ay","az","gx","gy","gz"]
    stats = ["mean","std","max","min"]

    for s in sensors:
        for st in stats:
            columns.append(f"{s}_{st}")

    columns.append("label")

    feature_df = pd.DataFrame(features, columns=columns)

    feature_df.to_csv(OUTPUT_FILE, index=False)

    print("Feature dataset created!")
    print("Total windows:", len(feature_df))


if __name__ == "__main__":
    main()