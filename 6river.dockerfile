FROM #DOCKER_IMAGE

RUN apt-get update && \
      apt-get install -y \
      build-essential \
      libffi-dev
