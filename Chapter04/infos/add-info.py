import json
import random
import argparse
import os
import math


def generate_new_providers(n):
    costs = [round(random.uniform(0.1, 1), 1) for _ in range(n)]
    providers = {
        "prices": [round(cost + 0.1, 1) for cost in costs],
        "amounts": [random.randint(100, 1000) for _ in range(n)],
        "reputations": [round(random.uniform(0.1, 1.0), 1) for _ in range(n)],
        "costs": costs
    }
    return providers


def generate_new_requesters(n):
    requesters = {
        "lambda1s": [random.randint(10, 100) for _ in range(n)],
        "lambda2s": [round(random.uniform(1, 10), 1) for _ in range(n)],
        "sensitivities": [round(random.uniform(0.1, 1.0), 1) for _ in range(n)]
    }
    return requesters


def main():
    parser = argparse.ArgumentParser(description="add providers/requesters proportionally for the convergence results")
    parser.add_argument("-i", "--input", required=True, help="input json file")
    parser.add_argument("-o", "--output", help="output json file")
    parser.add_argument("-r", "--ratio", type=float, required=True,
                        help="addition ratio, e.g. 0.1 = 10%")
    parser.add_argument("-m", "--mode", required=True,
                        choices=["provider", "requester", "all"],
                        help="addition object: provider/requester/all")

    args = parser.parse_args()

    input_file = args.input
    output_file = args.output or os.path.splitext(input_file)[0] + f"_expand_{args.mode}.json"
    ratio = args.ratio
    mode = args.mode

    with open(input_file, "r") as f:
        data = json.load(f)

    num_providers = len(data["providers"]["prices"])
    num_requesters = len(data["requesters"]["lambda1s"])

    add_providers_n = round(num_providers * ratio)
    add_requesters_n = round(num_requesters * ratio)

    if mode in ["provider", "all"] and add_providers_n > 0:
        new_p = generate_new_providers(add_providers_n)

        for key in data["providers"]:
            data["providers"][key].extend(new_p[key])

    if mode in ["requester", "all"] and add_requesters_n > 0:
        new_r = generate_new_requesters(add_requesters_n)

        for key in data["requesters"]:
            data["requesters"][key].extend(new_r[key])

    with open(output_file, "w") as f:
        json.dump(data, f, indent=4)

if __name__ == "__main__":
    main()
