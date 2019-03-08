pipeline {
    agent {
        docker { image 'maikenp/arc-sca-centos6' }
    }
    
    stages {
        stage('RATS') {
            steps {
                sh 'rats --quiet --html -w 3 ./ > rats.html'
                publishHTML (
                        target: [
                        allowMissing: true,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: '',
                        reportFiles: 'rats.html',
                        reportName: 'RATS Report'
                    ]) 
            }
        }
        stage('FLAWFINDER'){
            steps {
                sh '/flawfinder-1.31/flawfinder -m 2 --html --dataonly ./ > flawfinder.html 2>&1'
                publishHTML (
                        target: [
                        allowMissing: true,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: '',
                        reportFiles: 'flawfinder.html',
                        reportName: 'FLAWFINDER Report'
                    ]) 
            }
        }
        stage('CppCheck') {
            steps {
                    sh 'cppcheck --enable=all --inconclusive --xml --xml-version=2 . 2> cppcheck.xml'
                    sh 'scl enable python27 -- $CP_PROJECT_DIR/cppcheck/htmlreport/cppcheck-htmlreport --file=cppcheck.xml --report-dir=cppcheck_html --source-dir=.'
                    sh 'ls'
                    publishCppcheck pattern:'cppcheck.xml'
                    publishHTML (
                        target: [
                        allowMissing: true,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'cppcheck_html',
                        reportFiles: 'cppcheck.html',
                        reportName: 'CppCheck Report'
                    ]) 
            }
        }
    }
}
