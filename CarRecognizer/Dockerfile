FROM ghcr.io/astral-sh/uv:python3.13-bookworm-slim AS base
WORKDIR /app
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      libgl1 libglib2.0-0 libsm6 libxrender1 libxext6 \
 && rm -rf /var/lib/apt/lists/*

FROM base AS deps
WORKDIR /app
COPY pyproject.toml uv.lock* ./

RUN --mount=type=cache,target=/root/.cache/pip \
    --mount=type=cache,target=/root/.cache/uv \
    uv sync --frozen

FROM base
WORKDIR /app

COPY --from=deps /app /app

COPY . /app

CMD ["uv", "run", "car-recognizer"]
