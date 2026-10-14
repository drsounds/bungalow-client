import sys
import subprocess

# Start the background process (e.g., the IMD tracking service)
sidecar = subprocess.Popen(["bash ./spider.sh"])

# Start the main app
main_app = subprocess.Popen(sys.argv[1:])

# Wait for the main app to finish
main_app.wait()

# Kill the sidecar
sidecar.terminate()