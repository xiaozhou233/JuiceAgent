pipeline {
    agent any

    stages {
        stage('Build Image') {
            steps {
                script {
                    docker.build('juiceagent-builder:latest')
                }
            }
        }
        stage('Build') {
            steps {
                script {
                    docker.image('juiceagent-builder:latest').inside() {
                        sh 'rm -rf build-mingw'
                        sh 'cmake --preset mingw-release'
                        sh 'cmake --build build-mingw'
                    }
                }
            }
        }
    }

    post {
        success {
            archiveArtifacts artifacts: 'build-mingw/bin/*.exe,build-mingw/bin/*.dll', fingerprint: true, allowEmptyArchive: false
        }
    }
}
