import os
import pandas as pd

DATASET_PATH = "../data/raw/SisFall"
OUTPUT_FILE = "../data/processed/labeled_sensor_data.csv"

data = []


def get_label(filename):

    if filename.startswith("D"):
        return 0

    if filename.startswith("F"):
        return 1

    return None


def read_file(filepath, label):

    with open(filepath, "r") as f:

        for line in f:

            values = line.strip().split(",")

            try:
                ax = float(values[0])
                ay = float(values[1])
                az = float(values[2])
                gx = float(values[3])
                gy = float(values[4])
                gz = float(values[5])

                data.append([ax, ay, az, gx, gy, gz, label])

            except:
                continue


def main():

    print("Reading SisFall dataset...")

    normal_files = 0
    fall_files = 0

    for root, dirs, files in os.walk(DATASET_PATH):

        for file in files:

            if not file.endswith(".txt"):
                continue

            label = get_label(file)

            if label is None:
                continue

            if label == 0:
                normal_files += 1
            else:
                fall_files += 1

            filepath = os.path.join(root, file)

            read_file(filepath, label)

    df = pd.DataFrame(
        data,
        columns=["ax","ay","az","gx","gy","gz","label"]
    )

    os.makedirs("../data/processed", exist_ok=True)

    df.to_csv(OUTPUT_FILE, index=False)

    print("\nDataset created successfully")
    print("Normal files:", normal_files)
    print("Fall files:", fall_files)
    print("Total samples:", len(df))


if __name__ == "__main__":
    main()