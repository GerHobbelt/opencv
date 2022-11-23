#!/usr/bin/env groovy

def server = Artifactory.server 'sixriver'
parallel(
    failFast: true,
     "amd64-bionic": {
        node('docker && amd64') {
          def customImage = ""
          def scmVars = ""
          try {
            // uncomment this pre-clean stage if the workspace gets messed up
            // and the pre-checkout cleanup fails
            // stage("pre-clean") {
            //     docker.image("debian:stable-slim").inside('-u 0:0') {
            //         sh "pwd"
            //         sh "ls -al"
            //         sh "chmod -R a+rwX ."
            //     }
            // }
            stage("Build Docker Image") {
                scmVars = checkout scm
                sh 'sed -e "s/#DOCKER_IMAGE/ros:melodic/g" 6river.dockerfile > 6river-amd64.dockerfile'
                customImage = docker.build("gcr.io/plasma-column-128721/opencv-builder:amd64", " --file 6river-amd64.dockerfile ." )
            }
            stage("Build and Publish") {
              customImage.inside("-u 0:0 -e GIT_BRANCH=${scmVars.GIT_BRANCH}") {
                withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: 'artifactory_apt',
                        usernameVariable: 'ARTIFACTORY_USERNAME', passwordVariable: 'ARTIFACTORY_PASSWORD']]) {
                    withCredentials([string(credentialsId: 'github-access-token', variable: 'GITHUB_TOKEN')]) {
                      sh '''
                        export ARCHITECTURE='amd64'
                        export DISTRO='bionic'
                        ./install.sh
                      '''
                    }
                  }
                }

            }
          } catch (e) {
            echo 'Build failed'
            // Since we're catching the exception in order to report on it,
            // we need to re-throw it, to ensure that the build is marked as failed
            throw e
          } finally {
            stage("Cleanup") {
              customImage.inside('-u 0:0') {
                sh "pwd"
                sh "ls -al"
                sh "chmod -R a+rwX ."
              }
            }
          }
        }
    },
    "amd64-xenial": {
        node('docker && amd64') {
          def customImage = ""
          def scmVars = ""
          try {
            // uncomment this pre-clean stage if the workspace gets messed up
            // and the pre-checkout cleanup fails
            // stage("pre-clean") {
            //     docker.image("debian:stable-slim").inside('-u 0:0') {
            //         sh "pwd"
            //         sh "ls -al"
            //         sh "chmod -R a+rwX ."
            //     }
            // }
            stage("Build Docker Image") {
                scmVars = checkout scm
                sh 'sed -e "s/#DOCKER_IMAGE/ros:kinetic/g" 6river.dockerfile > 6river-amd64.dockerfile'
                customImage = docker.build("gcr.io/plasma-column-128721/opencv-builder:amd64", " --file 6river-amd64.dockerfile ." )
            }
            stage("Build and Publish") {
              customImage.inside("-u 0:0 -e GIT_BRANCH=${scmVars.GIT_BRANCH}") {
                withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: 'artifactory_apt',
                        usernameVariable: 'ARTIFACTORY_USERNAME', passwordVariable: 'ARTIFACTORY_PASSWORD']]) {
                    withCredentials([string(credentialsId: 'github-access-token', variable: 'GITHUB_TOKEN')]) {
                      sh '''
                        export ARCHITECTURE='amd64'
                        export DISTRO='xenial'
                        ./install.sh
                      '''
                    }
                  }
                }

            }
          } catch (e) {
            echo 'Build failed'
            // Since we're catching the exception in order to report on it,
            // we need to re-throw it, to ensure that the build is marked as failed
            throw e
          } finally {
            stage("Cleanup") {
              customImage.inside('-u 0:0') {
                sh "pwd"
                sh "ls -al"
                sh "chmod -R a+rwX ."
              }
            }
          }
        }
    },
    "arm64-xenial": {
        node('docker && arm64') {
          def customImage = ""
          def scmVars = ""
          try {
            // uncomment this pre-clean stage if the workspace gets messed up
            // and the pre-checkout cleanup fails
            // stage("pre-clean") {
            //     docker.image("debian:stable-slim").inside('-u 0:0') {
            //         sh "pwd"
            //         sh "ls -al"
            //         sh "chmod -R a+rwX ."
            //     }
            // }
            stage("Build Docker Image") {
                scmVars = checkout scm
                sh 'sed -e "s/#DOCKER_IMAGE/arm64v8\\/ros:kinetic/g" 6river.dockerfile > 6river-arm64.dockerfile'
                customImage = docker.build("gcr.io/plasma-column-128721/opencv-builder:arm64", " --file 6river-arm64.dockerfile ." )
            }
            stage("Build and Publish") {
              customImage.inside("-u 0:0 -e GIT_BRANCH=${scmVars.GIT_BRANCH}") {
                withCredentials([string(credentialsId: 'github-access-token', variable: 'GITHUB_TOKEN')]) {
                  withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: 'artifactory_apt',
                          usernameVariable: 'ARTIFACTORY_USERNAME', passwordVariable: 'ARTIFACTORY_PASSWORD']]) {
                      sh '''
                      export ARCHITECTURE='arm64'
                      export DISTRO='xenial'
                      ./install.sh
                      '''
                  }
                }
              }
            }
          } catch (e) {
            echo 'Build failed'
            // Since we're catching the exception in order to report on it,
            // we need to re-throw it, to ensure that the build is marked as failed
            throw e
          } finally {
            stage("Cleanup") {
              customImage.inside('-u 0:0') {
                sh "pwd"
                sh "ls -al"
                sh "chmod -R a+rwX ."
              }
            }
          }
        }
    },
    "arm64-bionic": {
        node('docker && arm64') {
          def customImage = ""
          def scmVars = ""
          try {
            // uncomment this pre-clean stage if the workspace gets messed up
            // and the pre-checkout cleanup fails
            // stage("pre-clean") {
            //     docker.image("debian:stable-slim").inside('-u 0:0') {
            //         sh "pwd"
            //         sh "ls -al"
            //         sh "chmod -R a+rwX ."
            //     }
            // }
            stage("Build Docker Image") {
                scmVars = checkout scm
                sh 'sed -e "s/#DOCKER_IMAGE/arm64v8\\/ros:melodic/g" 6river.dockerfile > 6river-arm64.dockerfile'
                customImage = docker.build("gcr.io/plasma-column-128721/opencv-builder:arm64", " --file 6river-arm64.dockerfile ." )
            }
            stage("Build and Publish") {
              customImage.inside("-u 0:0 -e GIT_BRANCH=${scmVars.GIT_BRANCH}") {
                withCredentials([string(credentialsId: 'github-access-token', variable: 'GITHUB_TOKEN')]) {
                  withCredentials([[$class: 'UsernamePasswordMultiBinding', credentialsId: 'artifactory_apt',
                          usernameVariable: 'ARTIFACTORY_USERNAME', passwordVariable: 'ARTIFACTORY_PASSWORD']]) {
                      sh '''
                      export ARCHITECTURE='arm64'
                      export DISTRO='bionic'
                      ./install.sh
                      '''
                  }
                }
              }
            }
          } catch (e) {
            echo 'Build failed'
            // Since we're catching the exception in order to report on it,
            // we need to re-throw it, to ensure that the build is marked as failed
            throw e
          } finally {
            stage("Cleanup") {
              customImage.inside('-u 0:0') {
                sh "pwd"
                sh "ls -al"
                sh "chmod -R a+rwX ."
              }
            }
          }
        }
    },
)
