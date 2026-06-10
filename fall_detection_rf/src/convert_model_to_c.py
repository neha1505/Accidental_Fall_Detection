import joblib
import numpy as np

MODEL_PATH = "../models/random_forest_model.pkl"
OUTPUT_PATH = "../models/rf_model.h"


def tree_to_c(tree, tree_id):

    left = tree.children_left
    right = tree.children_right
    threshold = tree.threshold
    feature = tree.feature
    value = tree.value

    code = ""

    def recurse(node, depth):

        indent = "    " * depth

        if threshold[node] != -2:

            code_line = f"{indent}if (features[{feature[node]}] <= {threshold[node]}) {{\n"
            code_line += recurse(left[node], depth + 1)
            code_line += f"{indent}}} else {{\n"
            code_line += recurse(right[node], depth + 1)
            code_line += f"{indent}}}\n"

            return code_line

        else:

            class_id = np.argmax(value[node][0])
            return f"{indent}return {class_id};\n"

    code += f"int tree_{tree_id}(float features[]) {{\n"
    code += recurse(0, 1)
    code += "}\n\n"

    return code


def main():

    print("Loading model...")

    model = joblib.load(MODEL_PATH)

    trees_code = ""

    for i, estimator in enumerate(model.estimators_):

        trees_code += tree_to_c(estimator.tree_, i)

    header = """
#ifndef RF_MODEL_H
#define RF_MODEL_H

"""

    voting_function = "int rf_predict(float features[]) {\n"
    voting_function += "    int votes0 = 0;\n"
    voting_function += "    int votes1 = 0;\n\n"

    for i in range(len(model.estimators_)):

        voting_function += f"    if (tree_{i}(features) == 0) votes0++;\n"
        voting_function += f"    else votes1++;\n\n"

    voting_function += """
    if (votes1 > votes0)
        return 1;
    else
        return 0;
}

#endif
"""

    with open(OUTPUT_PATH, "w") as f:

        f.write(header)
        f.write(trees_code)
        f.write(voting_function)

    print("Conversion complete!")
    print("Saved to:", OUTPUT_PATH)


if __name__ == "__main__":
    main()