from test_web_config import ROOT, extract, run_c


def test_wizard_avatar_metadata_uses_the_native_choice_and_screen():
    source=(ROOT/'engine/code/q3_ui/ui_rally_profile_wizard.c').read_text()
    harness=r'''
#include <assert.h>
#include <string.h>
#define PW_PAGE_DETAILS 1
#define PW_AVATAR_COUNT 3
typedef struct {int id;} menucommon_s;
static struct {int page;struct {menucommon_s generic;}avatarSpin;}s_pw;
static const char *s_avatarShaderPaths[]={"","gfx/avatars/one","gfx/avatars/two"};
static int active=1;
static int UI_WebProfileWizardMenu(void){return active;}
'''+extract(source,'const char *UI_WebProfileWizardAvatar(')+r'''
int main(void){menucommon_s other={0};s_pw.page=PW_PAGE_DETAILS;
assert(!strcmp(UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,1),s_avatarShaderPaths[1]));
assert(!strcmp(UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,2),s_avatarShaderPaths[2]));
assert(!*UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,0));
assert(!*UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,-1));
assert(!*UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,PW_AVATAR_COUNT));
assert(!*UI_WebProfileWizardAvatar(&other,1));
active=0;assert(!*UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,1));
active=1;s_pw.page=0;assert(!*UI_WebProfileWizardAvatar(&s_pw.avatarSpin.generic,1));
}
'''
    run_c(harness)
