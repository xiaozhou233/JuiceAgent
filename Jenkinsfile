pipeline {
    agent any

    environment {
        DOCKER_IMAGE = 'juiceagent-builder:latest'
    }

    stages {
        stage('Build Image') {
            steps {
                sh 'docker build -t juiceagent-builder:latest .'
            }
        }
        stage('Build') {
            steps {
                sh 'docker run --rm -v "$WORKSPACE:/workspace" juiceagent-builder:latest bash -lc "cd /workspace && rm -rf build-mingw && cmake --preset mingw-release && cmake --build build-mingw && cd build-mingw/bin && zip -9 -j /workspace/JuiceAgent-windows-x64.zip *.exe *.dll"'
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: 'JuiceAgent-windows-x64.zip', fingerprint: true, allowEmptyArchive: false
        }
    }
}
