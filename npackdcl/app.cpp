#include "math.h"

#include "app.h"
#include "..\wpmcpp\wpmutils.h"
#include "..\wpmcpp\commandline.h"

void App::jobChanged(const JobState& s)
{
    HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    time_t now = time(0);
    if (!s.completed) {
        if (now - this->lastJobChange != 0) {
            int w = progressPos.dwSize.X - 6;

            SetConsoleCursorPosition(hOutputHandle,
                    progressPos.dwCursorPosition);
            QString txt = s.hint;
            if (txt.length() >= w)
                txt = "..." + txt.right(w - 3);
            if (txt.length() < w)
                txt = txt + QString().fill(' ', w - txt.length());
            txt += QString("%1%").arg(floor(s.progress * 100 + 0.5), 4);
            std::cout << qPrintable(txt);
        }
    } else {
        QString filled;
        filled.fill(' ', progressPos.dwSize.X - 1);
        SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
        std::cout << qPrintable(filled);
        SetConsoleCursorPosition(hOutputHandle, progressPos.dwCursorPosition);
    }
}

int App::unitTests(int argc, char *argv[])
{
    std::cout << "Starting internal tests" << std::endl;

    Repository* r = new Repository();
    Job* job = new Job();
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    QFile f("..\\TestDependsOnItself.xml");
    if (!f.open(QIODevice::ReadOnly))
        std::cout << "Cannot open the file" << std::endl;
    doc.setContent(&f, false, &errorMsg, &errorLine, &errorColumn);
    r->loadOne(&doc, job);
    std::cout << r->packageVersions.size() << std::endl;
    if (!job->getErrorMessage().isEmpty())
        std::cout << "Error: " << qPrintable(job->getErrorMessage()) << std::endl;
    delete job;
    delete r;

    std::cout << "Internal tests were successful" << std::endl;

    return 0;
}

int App::process(int argc, char *argv[])
{
    cl.add("package", "internal package name (e.g. com.example.Editor)",
            "package", false);
    cl.add("versions", "versions range (e.g. [1.5,2))",
            "range", false);
    cl.add("version", "version number (e.g. 1.5.12)",
            "version", false);
    QString err = cl.parse(argc, argv);
    if (!err.isEmpty()) {
        std::cout << "Error: " << qPrintable(err) << std::endl;
        return 1;
    }
    // cl.dump();

    int r = 0;

    QStringList fr = cl.getFreeArguments();

    if (fr.count() == 0) {
        std::cerr << "Missing command. Try npackdcl help" << std::endl;
        r = 1;
    } else if (fr.count() > 1) {
        std::cerr << "Unexpected argument: " << qPrintable(fr.at(1)) << std::endl;
        r = 1;
    } else if (fr.at(0) == "help") {
        usage();
    } else if (fr.at(0) == "path") {
        r = path();
    } else if (fr.at(0) == "remove") {
        r = remove();
    } else if (fr.at(0) == "add") {
        r = add();
    /*} else if (params.count() == 2 && params.at(1) == "list") {
        QList<PackageVersion*> installed = rep->packageVersions; // getInstalled();
        for (int i = 0; i < installed.count(); i++) {
            std::cout << qPrintable(installed.at(i)->getPackageTitle()) << " " <<
                    qPrintable(installed.at(i)->version.getVersionString()) <<
                    " " << qPrintable(installed.at(i)->getPath()) <<
                    std::endl;
        }
    } else if (params.count() == 2 && params.at(1) == "info") {
        / *std::cout << "Installation directory: " <<
                qPrintable(rep->getDirectory().absolutePath()) << std::endl;
        QList<PackageVersion*> installed = rep->getInstalled();
        std::cout << "Number of installed packages: " <<
                installed.count() << std::endl;*/
    } else {
        std::cerr << "Wrong command: " << qPrintable(fr.at(0)) << std::endl;
        usage();
        r = 1;
    }

    return r;
}

void App::addNpackdCL()
{
    Repository* r = Repository::getDefault();
    PackageVersion* pv = r->findOrCreatePackageVersion(
            "com.googlecode.windows-package-manager.NpackdCL",
            Version(WPMUtils::NPACKD_VERSION));
    if (!pv->installed()) {
        pv->setPath(WPMUtils::getExeDir());
        pv->setExternal(true);
        r->updateNpackdCLEnvVar();
    }
}

Job* App::createJob()
{
    HANDLE hOutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hOutputHandle, &progressPos);
    if (progressPos.dwCursorPosition.Y >= progressPos.dwSize.Y - 1) {
        std::cout << std::endl;
        progressPos.dwCursorPosition.Y--;
    }

    Job* job = new Job();
    connect(job, SIGNAL(changed(const JobState&)), this,
            SLOT(jobChanged(const JobState&)));

    // -1 so that we do not have the initial 1 second delay
    this->lastJobChange = time(0) - 1;

    return job;
}

void App::usage()
{
    std::cout << "Npackd command line tool" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "    npackdcl help" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl path --package=<package> --versions=<versions>" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl add --package=<package> --version=<version>" << std::endl;
    std::cout << "    or" << std::endl;
    std::cout << "    npackdcl remove --package=<package> --version=<version>" << std::endl;
    std::cout << std::endl;
    std::cout << "You can use short package names in 'add' and 'remove' operations." << std::endl;
    std::cout << "Example: App instead of com.example.App" << std::endl;
    std::cout << "The process exits with the code unequal to 0 if an error occcures." << std::endl;
    /*
    std::cout << "Usage: npackdcl list" << std::endl;
    std::cout << "or" << std::endl;
    std::cout << "Usage: npackdcl info" << std::endl;*/
}

int App::path()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = new Job();
    rep->refresh(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString versions = cl.get("versions");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        if (versions.isNull()) {
            std::cerr << "Missing option: --versions" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        if (!Package::isValidName(package)) {
            std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
        Dependency d;
        d.package = package;
        if (!d.setVersions(versions)) {
            std::cerr << "Cannot parse versions: " << qPrintable(versions) << std::endl;
            usage();
            r = 1;
        } else {
            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = d.findHighestInstalledMatch();
            if (pv) {
                QString p = pv->getPath();
                p.replace('/', '\\');
                std::cout << qPrintable(p) << std::endl;
            }
        }
    }

    return r;
}

int App::add()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        if (version.isNull()) {
            std::cerr << "Missing option: --versions" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                std::cerr << "Ambiguous package name" << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            Version v;
            if (!v.setVersion(version)) {
                std::cerr << "Cannot parse version: " << qPrintable(version) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(
                    packages.at(0)->name, version);
            if (!pv) {
                std::cerr << "Package version not found" << std::endl;
                r = 1;
                break;
            }
            if (pv->installed()) {
                std::cerr << "Package is already installed in " <<
                        qPrintable(pv->getPath()) << std::endl;
                r = 1;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QString err = pv->planInstallation(installed, ops);
            if (!err.isEmpty()) {
                std::cerr << qPrintable(err) << std::endl;
                r = 1;
                break;
            }

            Job* ijob = createJob();
            rep->process(ijob, ops);
            if (!ijob->getErrorMessage().isEmpty()) {
                std::cerr << qPrintable(ijob->getErrorMessage()) << std::endl;
                r = 1;
            }
            delete ijob;
        } while (false);
    }

    return r;
}

int App::remove()
{
    int r = 0;

    Repository* rep = Repository::getDefault();
    Job* job = createJob();
    rep->reload(job);
    if (!job->getErrorMessage().isEmpty()) {
        std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
        r = 1;
    }
    delete job;

    addNpackdCL();

    QString package = cl.get("package");
    QString version = cl.get("version");

    if (r == 0) {
        if (package.isNull()) {
            std::cerr << "Missing option: --package" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        if (version.isNull()) {
            std::cerr << "Missing option: --versions" << std::endl;
            usage();
            r = 1;
        }
    }

    if (r == 0) {
        do {
            if (!Package::isValidName(package)) {
                std::cerr << "Invalid package name: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }

            Repository* rep = Repository::getDefault();
            QList<Package*> packages = rep->findPackages(package);
            if (packages.count() == 0) {
                std::cerr << "Unknown package: " << qPrintable(package) << std::endl;
                r = 1;
                break;
            }
            if (packages.count() > 1) {
                std::cerr << "Ambiguous package name" << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout <<  qPrintable(package) << " " << qPrintable(versions);
            Version v;
            if (!v.setVersion(version)) {
                std::cerr << "Cannot parse version: " << qPrintable(version) << std::endl;
                r = 1;
                break;
            }

            // debug: std::cout << "Versions: " << qPrintable(d.toString()) << std::endl;
            PackageVersion* pv = rep->findPackageVersion(
                    packages.at(0)->name, version);
            if (!pv) {
                std::cerr << "Package not found" << std::endl;
                r = 1;
                break;
            }

            if (!pv->installed()) {
                std::cerr << "Package is not installed" << std::endl;
                r = 1;
                break;
            }

            if (pv->isExternal()) {
                std::cerr << "Externally installed packages cannot be removed" << std::endl;
                r = 1;
                break;
            }

            QList<InstallOperation*> ops;
            QList<PackageVersion*> installed =
                    Repository::getDefault()->getInstalled();
            QString err = pv->planUninstallation(installed, ops);
            if (!err.isEmpty()) {
                std::cerr << qPrintable(err) << std::endl;
                r = 1;
                break;
            }

            Job* job = createJob();
            rep->process(job, ops);
            if (!job->getErrorMessage().isEmpty()) {
                std::cerr << qPrintable(job->getErrorMessage()) << std::endl;
                r = 1;
            }
            delete job;
        } while (false);
    }

    return r;
}
