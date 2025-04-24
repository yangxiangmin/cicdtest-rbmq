pipeline {
    agent any
    
    environment {
        RABBITMQ_HOST = 'localhost'
        RABBITMQ_PORT = '5672'
        RABBITMQ_USER = 'guest'
        RABBITMQ_PASS = 'guest'
        BUILD_DIR = "${WORKSPACE}/build"
        REPORT_DIR = "${WORKSPACE}/reports"
        ARTIFACT_DIR = "${WORKSPACE}/artifacts"
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
                cd ${BUILD_DIR}
                ls -lR  # 递归列出所有文件
                '''
                echo "✅ 已完成编译！"
            }
        }
        
        stage('Unit Test') {
/*
            steps {
                sh '''
                mkdir -p ${REPORT_DIR}
                cd ${BUILD_DIR}
                ctest --output-on-failure --no-compress-output -T Test
                '''
                echo "✅ 已完成单元测试！"
            }
*/
            steps {
                sh '''
                cd ${BUILD_DIR}
                ./tests/test_rabbitmq_wrapper --gtest_output="xml:${WORKSPACE}/${BUILD_DIR}/test-results.xml"
                ls -l "${WORKSPACE}/${BUILD_DIR}/test-results.xml" || echo "❌ 报告生成失败"
                '''
                junit "${BUILD_DIR}/test-results.xml"
                echo "✅ 已完成测试！"
            }
        }
 /*       
        stage('Integration Test') {
            steps {
                sh '''
                # 启动测试RabbitMQ服务器
                docker run -d --name rabbitmq-test -p 5672:5672 rabbitmq:3-management
                sleep 30 # 等待RabbitMQ启动
                
                # 运行集成测试
                cd ${BUILD_DIR}
                ./test/integration/integration_test_runner
                '''
                echo "✅ 已完成集成测试！"
            }
        }

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
                mkdir -p ${ARTIFACT_DIR}
                # 创建Debian包
                cd ${BUILD_DIR}
                cpack -G DEB
                cp *.deb ${ARTIFACT_DIR}
                
                # 创建Docker镜像
                docker build -t rabbitmq-wrapper:${VERSION} .
                docker save rabbitmq-wrapper:${VERSION} > ${ARTIFACT_DIR}/rabbitmq-wrapper-${VERSION}.tar
                '''
                echo "✅ 已完成打包！"
            }
        }
        
        stage('Deploy to Staging') {
            when {
                branch 'main'
            }
            steps {
                sh '''
                # 部署到测试环境
                scp ${ARTIFACT_DIR}/*.deb staging-server:/tmp/
                ssh staging-server "sudo dpkg -i /tmp/rabbitmq-wrapper-*.deb && sudo systemctl restart rabbitmq-wrapper"
                '''
                echo "✅ 已部署到预发布环境！"
            }
        }
        
        stage('Deploy to Production') {
            when {
                branch 'release/*'
            }
            steps {
                input message: 'Deploy to production?', ok: 'Deploy'
                sh '''
                # 部署到生产环境
                scp ${ARTIFACT_DIR}/*.deb production-server:/tmp/
                ssh production-server "sudo dpkg -i /tmp/rabbitmq-wrapper-*.deb && sudo systemctl restart rabbitmq-wrapper"
                '''
                echo "✅ 已部署到预生产环境！"
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
                    to: 'dev-team@your-org.com'
        }
        success {
            emailext body: '${DEFAULT_CONTENT}\n\n${BUILD_URL}', 
                    subject: 'SUCCESS: Job ${JOB_NAME} - Build ${BUILD_NUMBER}', 
                    to: 'dev-team@your-org.com'
        }
    }
}