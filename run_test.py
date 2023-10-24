import requests
import threading
import time

def make_request(url, max_retries=3):

    for attempt in range(max_retries):
        try:
            response = requests.get(url)
            return response.elapsed.total_seconds()
        except requests.exceptions.RequestException as e:
            print(f"Error: {e}. Retrying...")
            time.sleep(1)  # Wait for 1 second before retrying

    print(f"Failed to make the request after {max_retries} attempts.")
    return None

def run(url):
    start_time = time.time()

    threads = []
    for _ in range(100):
        thread = threading.Thread(target=lambda: make_request(url))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

    end_time = time.time()

    total_time = end_time - start_time
    return total_time

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Run web 100 times concurrently")
    parser.add_argument("url", type=str, help="url to use")
    args = parser.parse_args()
    server_url = args.url

    total_time = []

    for _ in range(1000):
        timer = run(server_url)
        print(f"Request {_ + 1}: {timer} seconds")
        total_time.append(timer)

    print(f"Average time for 1000 requests: {sum(total_time) / len(total_time)} seconds")
    print(total_time)
