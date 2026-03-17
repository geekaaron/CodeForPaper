import json
import random
import argparse
import os

def perturb_value(v, ratio, max_val=None):
    v_new = v + v * random.uniform(-ratio, ratio)
    v_new = max(v_new, 0.01)
    if max_val is not None:
        v_new = min(v_new, max_val)
    return round(v_new, 2)


def perturb_list(values, ratio, max_val=None):
    return [perturb_value(v, ratio, max_val) for v in values]


def main():
    parser = argparse.ArgumentParser(description="add perturbations to the game convergence data without changing the price")
    parser.add_argument("-i", "--input", required=True, help="input json file")
    parser.add_argument("-o", "--output", default=None, help="output json file")
    parser.add_argument("-r", "--ratio", type=float, default=0.05, help="Perturbation ratio")
    parser.add_argument("-m", "--mode", required=True, choices=["provider", "requester", "all"],
                        help="purturbation mode: provider/requester/all")

    args = parser.parse_args()

    input_file = args.input
    output_file = args.output or os.path.splitext(input_file)[0] + f"_perturbed_{args.mode}.json"
    ratio = args.ratio
    mode = args.mode

    with open(input_file, "r") as f:
        data = json.load(f)

    if mode in ["provider", "all"]:
        data["providers"]["amounts"] = perturb_list(data["providers"]["amounts"], ratio)
        data["providers"]["reputations"] = perturb_list(
            data["providers"]["reputations"], ratio, max_val=1.0
        )
        data["providers"]["costs"] = perturb_list(data["providers"]["costs"], ratio)

    if mode in ["requester", "all"]:
        data["requesters"]["lambda1s"] = perturb_list(data["requesters"]["lambda1s"], ratio)
        data["requesters"]["lambda2s"] = perturb_list(data["requesters"]["lambda2s"], ratio)
        data["requesters"]["sensitivities"] = perturb_list(
            data["requesters"]["sensitivities"], ratio, max_val=1.0
        )

    with open(output_file, "w") as f:
        json.dump(data, f, indent=4)

if __name__ == "__main__":
    main()
