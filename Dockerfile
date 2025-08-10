FROM ubuntu:22.04

WORKDIR /app

# Install system dependencies
RUN apt-get update && \
    apt-get install -y git build-essential cmake libserial-dev libzmq3-dev python3 python3-pip && \
    rm -rf /var/lib/apt/lists/*

# Copy dependency definitions and install Python deps
COPY requirements.txt ./
RUN pip3 install --no-cache-dir -r requirements.txt

# Copy source code
COPY . .

# Fetch submodules and build the project
RUN git submodule update --init --recursive \
    && ./scripts/ds_build_livoxsdk2.sh \
    && ./scripts/ds_build_app.sh

# Expose the web interface port
EXPOSE 5000

CMD ["./scripts/run_web.sh"]
