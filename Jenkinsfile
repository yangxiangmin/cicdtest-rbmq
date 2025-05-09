pipeline {
    agent any
    
    environment {
        RABBITMQ_HOST = 'localhost'
        RABBITMQ_PORT = '5672'
        RABBITMQ_USER = 'guest'
        RABBITMQ_PASS = 'guest'
        BUILD_DIR = "build"
        ARTIFACTS_DIR = "artifacts"
        VERSION = sh(script: 'git describe --tags --always', returnStdout: true).trim()
    }
    
    stages {
        stage('Checkout') {
            steps {
                git branch: 'main', 
                    url: 'https://github.com/yangxiangmin/cicdtest-rbmq.git',
                    credentialsId: 'github-credentials'
                
                // 生成版本信息文件
                sh '''
                echo "#define VERSION \\"RabbitMQWrapper Version ${VERSION}\\"" > include/version.h.in
                echo "#define CMP_IPADDR \\"${NODE_NAME}\\"" >> include/version.h.in
                echo "#define BUILD_TIME \\"`date +"%Y-%m-%d %H:%M:%S"`\\"" >> include/version.h.in
                echo "#define ARCH_TYPE \\"`uname -m`\\"" >> include/version.h.in
                '''
                echo "✅ 已完成代码检出！"
            }
        }

        stage('Install Dependencies') {
            steps {
                sh '''
                # 检测操作系统类型
                if [ -f /etc/os-release ]; then
                    # RedHat/CentOS 系统
                    sudo yum install -y cmake gcc-c++ openssl-devel libcurl-devel jsoncpp-devel

                    if [ ! -f /usr/local/lib/libgtest.a ] && [ ! -f /usr/local/lib64/libgtest.a ]; then
                        echo "从源码编译安装 gtest..."
                        GTEST_DIR="/usr/local/src/gtest"
                        sudo mkdir -p $GTEST_DIR
                        sudo git clone https://github.com/google/googletest.git $GTEST_DIR
                        cd $GTEST_DIR
                        sudo cmake .
                        sudo make
                        sudo make install
                    else
                        echo "gtest 已经安装"
                    fi

                    if ([ ! -f /usr/lib/librabbitmq.a ] && [ ! -f /usr/lib64/librabbitmq.a ]) || ([ ! -f /usr/lib/librabbitmq.so ] && [ ! -f /usr/lib64/librabbitmq.so ]); then
                        echo "从源码编译安装 rabbitmq..."
                        git clone https://github.com/alanxz/rabbitmq-c.git
                        cd rabbitmq-c
                        mkdir build && cd build
                        # cmake -DCMAKE_INSTALL_PREFIX=/usr ..
                        cmake ..
                        make
                        sudo make install
                    else
                        echo "rabbitmq 已经安装"
                    fi

                elif [ -f /etc/debian_version ]; then
                    # Debian/Ubuntu 系统
                    sudo apt-get update
                    sudo apt-get install -y cmake g++ librabbitmq-dev libssl-dev libgtest-dev libcurl-devel libjsoncpp-dev
                    # 编译安装 gtest
                    cd /usr/src/gtest
                    sudo cmake CMakeLists.txt
                    sudo make
                    sudo cp *.a /usr/lib
                    
                else
                    echo "Error: Unsupported operating system"
                    exit 1
                fi
                '''
                echo "✅ 已完成依赖环境安装！"
            }
        }
        
        stage('Build') {
            steps {
                sh '''
                mkdir -p ${BUILD_DIR}
                cd ${BUILD_DIR}
                cmake -DCMAKE_BUILD_TYPE=Release ..
                make -j4
                '''
                echo "✅ 已完成编译！"
            }
        }
        
        stage('Unit Test') {
            steps {
                sh '''
                cd ${BUILD_DIR}
                ./test/unit/test_rabbitmq_wrapper --gtest_output="xml:${WORKSPACE}/${BUILD_DIR}/test-results.xml"
                ls -l "${WORKSPACE}/${BUILD_DIR}/test-results.xml" || echo "❌ 报告生成失败"
                '''
                junit "${BUILD_DIR}/test-results.xml"
                echo "✅ 已完成单元测试！"
            }
        }
 
        stage('Integration Test') {
            steps {
                sh '''
                cd ${BUILD_DIR}
                ./test/integration/integration_test_runner --gtest_output="xml:${WORKSPACE}/${BUILD_DIR}/test-results.xml"
                ls -l "${WORKSPACE}/${BUILD_DIR}/test-results.xml" || echo "❌ 报告生成失败"
                '''
                junit "${BUILD_DIR}/test-results.xml"
                echo "✅ 已完成集成测试！"
            }
        }

/*
        stage('System Test') {
            steps {
                sh '''
                # 启动系统测试环境
                docker-compose -f test/system/docker-compose.yml up -d
                sleep 20
                
                # 运行系统测试
                cd ${BUILD_DIR}
                ./test/system/system_test_runner
                '''
                echo "✅ 已完成系统测试！"
            }
        }
 */
         stage('Package') {
            steps {
                sh '''
                mkdir -p ${ARTIFACTS_DIR}
                cp ${BUILD_DIR}/*.so ${ARTIFACTS_DIR}/
                tar -czvf rabbitmq_ops-$(date +%Y%m%d).tar.gz ${ARTIFACTS_DIR}
                '''
                archiveArtifacts artifacts: '*.tar.gz'
                echo "✅ 已完成打包！"
            }
        }

        stage('Deploy-Test') {
            // when { branch 'main' }
            steps {
                sh 'echo "当前分支是：$(git rev-parse --abbrev-ref HEAD)"'
                sshPublisher(
                    publishers: [
                        sshPublisherDesc(
                            configName: 'testenv', 
                            transfers: [
                                sshTransfer(
                                    sourceFiles: 'rabbitmq_ops-*.tar.gz',
                                    removePrefix: '',
                                    remoteDirectory: '/opt/rabbitmq_ops',
                                    execCommand: '''
                                        cd /opt/rabbitmq_ops && \
                                        tar -xzvf rabbitmq_ops-*.tar.gz && \
                                        rm -f rabbitmq_ops-*.tar.gz
                                    '''
                                )
                            ],
                            usePromotionTimestamp: false,
                            useWorkspaceInPromotion: false,
                            verbose: true
                        )
                    ]
                )
                echo "✅ 已部署到预发布环境！"
            }
        }

        stage('Deploy-Pro') {
            // when { branch 'main' }
            steps {
                sh 'echo "当前分支是：$(git rev-parse --abbrev-ref HEAD)"'
                sshPublisher(
                    publishers: [
                        sshPublisherDesc(
                            configName: 'proenv', 
                            transfers: [
                                sshTransfer(
                                    sourceFiles: 'rabbitmq_ops-*.tar.gz',
                                    removePrefix: '',
                                    remoteDirectory: '/opt/rabbitmq_ops',
                                    execCommand: '''
                                        cd /opt/rabbitmq_ops && \
                                        tar -xzvf rabbitmq_ops-*.tar.gz && \
                                        rm -f rabbitmq_ops-*.tar.gz
                                    '''
                                )
                            ],
                            usePromotionTimestamp: false,
                            useWorkspaceInPromotion: false,
                            verbose: true
                        )
                    ]
                )
                echo "✅ 已部署到生产环境！"
            }
        }       
    }
    
    post {
        always {
            cleanWs()
        }
        failure {
            emailext body: '${DEFAULT_CONTENT}\n\n${BUILD_URL}', 
                    subject: 'FAILED: Job ${JOB_NAME} - Build ${BUILD_NUMBER}', 
                    to: 'yang.xiangmin@ghtchina.com'
        }
        success {
            emailext body: '${DEFAULT_CONTENT}\n\n${BUILD_URL}', 
                    subject: 'SUCCESS: Job ${JOB_NAME} - Build ${BUILD_NUMBER}', 
                    to: 'yang.xiangmin@ghtchina.com'
        }
    }
}