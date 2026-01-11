#!/usr/bin/env node

// Pre-build config script for postgres module
// This script outputs link configuration that will be propagated to dependents

const config = {
  link_configs: [
    {
      package: "mattn/postgres",
      link_libs: ["pq"]
    }
  ]
};

console.log(JSON.stringify(config));

