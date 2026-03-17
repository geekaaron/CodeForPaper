import json
import random
import argparse
import os

def generate_random_data(num_providers, num_requesters):
    costs = [round(random.uniform(0.1, 1), 1) for _ in range(num_providers)]
    providers = {
        "prices": [round(cost + 0.1, 1) for cost in costs],  # price = cost + 0.1
        "amounts": [random.randint(100, 1000) for _ in range(num_providers)],
        "reputations": [round(random.uniform(0.1, 1.0), 1) for _ in range(num_providers)],
        "costs": costs
    }


    requesters = {
        "lambda1s": [random.randint(10, 100) for _ in range(num_requesters)],
        "lambda2s": [round(random.uniform(1, 10), 1) for _ in range(num_requesters)],
        "sensitivities": [round(random.uniform(0.1, 1.0), 1) for _ in range(num_requesters)]
    }

    data = {
        "providers": providers,
        "requesters": requesters
    }

    return data

def main():
    parser = argparse.ArgumentParser(description="Generate random JSON data for providers and requesters.")
    parser.add_argument("-p", "--num_providers", type=int, required=True, help="Number of providers to generate.")
    parser.add_argument("-r", "--num_requesters", type=int, required=True, help="Number of requesters to generate.")
    parser.add_argument("-o", "--output_file", type=str, required=True, help="Output JSON file path.")

    args = parser.parse_args()

    num_providers = args.num_providers
    num_requesters = args.num_requesters
    output_file = args.output_file

    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    data = generate_random_data(num_providers, num_requesters)

    with open(output_file, "w") as json_file:
        json.dump(data, json_file, indent=4)

if __name__ == "__main__":
    main()
