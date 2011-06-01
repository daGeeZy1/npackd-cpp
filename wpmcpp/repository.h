#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <windows.h>

#include "qfile.h"
#include "qlist.h"
#include "qurl.h"
#include "qtemporaryfile.h"
#include "qdom.h"

#include "package.h"
#include "packageversion.h"
#include "license.h"
#include "node.h"
#include "digraph.h"
#include "windowsregistry.h"

/**
 * A repository is a list of packages and package versions.
 */
class Repository
{
private:
    static Repository def;

    static Package* createPackage(QDomElement* e);
    static PackageVersionFile* createPackageVersionFile(QDomElement* e);
    static Dependency* createDependency(QDomElement* e);
    static License* createLicense(QDomElement* e);
    static PackageVersion* createPackageVersion(QDomElement* e);
    static DetectFile* createDetectFile(QDomElement* e);

    void loadOne(QUrl* url, Job* job);

    void addWindowsPackage();

    void clearExternallyInstalled(QString package);

    void detectOneDotNet(const WindowsRegistry& wr, const QString& keyName);
    void detectMSIProducts();
    void detectDotNet();
    void detectMicrosoftInstaller();
    void detectMSXML();
    void detectJRE(bool w64bit);
    void detectJDK(bool w64bit);
    void detectWindows();

    /**
     * @param exact if true, only exact matches to packages from current
     *     repositories recognized as existing software (e.g. something like
     *     com.mysoftware.MySoftware-2.2.3). This setting should help in rare
     *     cases when Npackd 1.14 and 1.15 are used in parallel for some time
     *     If the value is false, also
     *     packages not known in current repositories are recognized as
     *     installed.
     */
    void scanPre1_15Dir(bool exact);

    /**
     * All paths should be in lower case
     * and separated with \ and not / and cannot end with \.
     *
     * @param path directory
     * @param ignore ignored directories
     */
    void scan(const QString& path, Job* job, int level, QStringList& ignore);

    /**
     * Loads the content from the URLs. None of the packages has the information
     * about installation path after this method was called.
     *
     * @param job job for this method
     */
    void load(Job* job);

    /**
     * Adds unknown in the repository, but installed packages.
     */
    void detectPre_1_15_Packages();

    void addWellKnownPackages();
public:
    /**
     * Package versions. All version numbers should be normalized.
     */
    QList<PackageVersion*> packageVersions;

    /**
     * Packages.
     */
    QList<Package*> packages;

    /**
     * Licenses.
     */
    QList<License*> licenses;

    /**
     * Creates an empty repository.
     */
    Repository();

    ~Repository();

    void process(Job* job, const QList<InstallOperation*> &install);

    /**
     * Loads one repository from an XML document.
     *
     * @param doc repository
     * @param job Job
     */
    void loadOne(QDomDocument* doc, Job* job);

    /**
     * Reads the package statuses from the registry.
     */
    void readRegistryDatabase();

    /**
     * Changes the value of the system-wide NPACKD_CL variable to point to the
     * newest installed version of NpackdCL.
     */
    void updateNpackdCLEnvVar();

    /**
     * @return new NPACKD_CL value
     */
    QString computeNpackdCLEnvVar();

    /**
     * Recognizes some applications installed without Npackd. This method does
     * not scan the hard drive and is fast.
     *
     * @param job job object
     */
    void recognize(Job* job);

    /**
     * Finds or creates a new package version.
     *
     * @param package package name
     * @param v found version
     * @return package version
     */
    PackageVersion* findOrCreatePackageVersion(const QString &package,
            const Version &v);

    /**
     * Finds all installed packages. This method lists all directories in the
     * installation directory and finds the corresponding package versions
     *
     * @return the list of installed package versions (the objects should not
     *     be freed)
     */
    QList<PackageVersion*> getInstalled();

    /**
     * @return digraph with installed package versions. Each Node.userData is
     *     of type PackageVersion* and represents an installed package version.
     *     The memory should not be freed. The first object in the list has
     *     the userData==0 and represents the user which "depends" on a list
     *     of packages (uses some programs).
     */
    Digraph* createInstalledGraph();

    /**
     * Counts the number of installed packages that can be updated.
     *
     * @return the number
     */
    int countUpdates();

    /**
     * Reloads all repositories.
     *
     * @param job job for this method
     */
    void reload(Job* job);

    /**
     * Reloads the database about installed packages from the
     * registry and performs a quick detection of packages.
     *
     * @param job job for this method
     */
    void refresh(Job* job);

    /**
     * Scans the hard drive for existing applications.
     *
     * @param job job for this method
     */
    void scanHardDrive(Job* job);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package or 0
     */
    Package* findPackage(const QString& name);

    /**
     * Searches for a package by name.
     *
     * @param name name of the package like "org.server.Word" or the short
     *     name "Word"
     * @return found packages
     */
    QList<Package*> findPackages(const QString& name);

    /**
     * Searches for a license by name.
     *
     * @param name name of the license like "org.gnu.GPLv3"
     * @return found license or 0
     */
    License* findLicense(const QString& name);

    /**
     * Find the newest installable package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestInstallablePackageVersion(const QString& name);

    /**
     * Find the newest installed package version.
     *
     * @param name name of the package like "org.server.Word"
     * @return found package version or 0
     */
    PackageVersion* findNewestInstalledPackageVersion(const QString& name);

    /**
     * Find the newest available package version.
     *
     * @param package name of the package like "org.server.Word"
     * @param version package version
     * @return found package version or 0
     */
    PackageVersion* findPackageVersion(const QString& package,
            const Version& version);

    /**
     * @return newly created object pointing to the repositories
     */
    static QList<QUrl*> getRepositoryURLs();

    /*
     * Changes the default repository url.
     *
     * @param urls new URLs
     */
    static void setRepositoryURLs(QList<QUrl*>& urls);

    /**
     * @return default repository
     */
    static Repository* getDefault();
};

#endif // REPOSITORY_H
