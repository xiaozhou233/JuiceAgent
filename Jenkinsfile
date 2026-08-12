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
        stage('Package') {
            steps {
                sh 'docker run --rm -v "$WORKSPACE:/workspace" juiceagent-builder:latest bash -lc "cd /workspace && bash tools/build-release.sh"'
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: 'JuiceAgent-windows-x64.zip, JuiceAgent-windows-x64-examples.zip', fingerprint: true, allowEmptyArchive: false
        }
    }
}
